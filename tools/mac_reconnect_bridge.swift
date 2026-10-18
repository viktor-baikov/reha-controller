import Foundation
import CoreBluetooth
import CoreMIDI

// Diagnostic app-owned transport. No Apple MIDI driver activation or UI automation.
final class ReconnectBridge: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    let serviceID = CBUUID(string: "03B80E5A-EDE8-4B33-A751-6CE34EC4C700")
    let characteristicID = CBUUID(string: "7772E5DB-3868-4112-A1A9-F2669D106BF3")
    let target: UUID
    var central: CBCentralManager!
    var peripheral: CBPeripheral?
    var characteristic: CBCharacteristic?
    var client = MIDIClientRef()
    var source = MIDIEndpointRef()
    var destination = MIDIEndpointRef()
    var ready = false
    var retry: Timer?
    var writes: [Data] = []
    var writing = false
    var heldNotes = Set<UInt8>()
    var feedbackDecoder = ChannelMIDIStream()
    var lastLoggedState: UInt8?

    init(target: UUID) {
        self.target = target
        super.init()
        func check(_ value: OSStatus) { precondition(value == noErr, "CoreMIDI error \(value)") }
        check(MIDIClientCreateWithBlock("Reha BLE transport" as CFString, &client) { _ in })
        check(MIDISourceCreate(client, "Reha Probe Bridge" as CFString, &source))
        check(MIDIDestinationCreateWithBlock(client, "Reha Probe Bridge" as CFString, &destination) { [weak self] list, _ in
            let offset = MemoryLayout<MIDIPacketList>.offset(of: \MIDIPacketList.packet)!
            var packet = UnsafeRawPointer(list).advanced(by: offset).assumingMemoryBound(to: MIDIPacket.self)
            var bytes: [UInt8] = []
            for _ in 0..<list.pointee.numPackets {
                let offset = MemoryLayout<MIDIPacket>.offset(of: \MIDIPacket.data)!
                let data = UnsafeRawPointer(packet).advanced(by: offset).assumingMemoryBound(to: UInt8.self)
                bytes.append(contentsOf: UnsafeBufferPointer(start: data, count: Int(packet.pointee.length)))
                packet = UnsafePointer(MIDIPacketNext(packet))
            }
            DispatchQueue.main.async {
                guard let self = self else { return }
                for message in self.feedbackDecoder.feed(bytes) { self.feedback(message) }
            }
        })
        // Deterministic IDs let the recorder remember these virtual ports across
        // connector restarts. Refuse collisions (including a second connector).
        func endpointID(_ role: String) -> Int32 {
            var hash: UInt32 = 2166136261
            for byte in "reha:\(target.uuidString):\(role)".utf8 { hash = (hash ^ UInt32(byte)) &* 16777619 }
            return Int32((hash & 0x7FFFFFFF) | 1)
        }
        for (endpoint, role) in [(source, "input"), (destination, "output")] {
            let status = MIDIObjectSetIntegerProperty(endpoint, kMIDIPropertyUniqueID, endpointID(role))
            guard status == noErr else {
                print("Cannot claim persistent MIDI port ID (\(status)); another bridge may be running")
                MIDIClientDispose(client)
                exit(3)
            }
        }
        central = CBCentralManager(delegate: self, queue: .main)
    }
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn { connect() }
        else { clear(); print("Bluetooth unavailable: \(central.state.rawValue)") }
    }
    func connect() {
        guard central.state == .poweredOn else { return }
        if let found = central.retrievePeripherals(withIdentifiers: [target]).first {
            peripheral = found
            found.delegate = self
            central.connect(found, options: nil) // pending connection waits for board
            print("Waiting for selected device \(target)")
        } else {
            central.scanForPeripherals(withServices: [serviceID], options: nil)
        }
    }
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi RSSI: NSNumber) {
        guard peripheral.identifier == target else { return }
        central.stopScan()
        self.peripheral = peripheral
        peripheral.delegate = self
        central.connect(peripheral, options: nil)
    }
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        retry?.invalidate()
        clear()
        print("BLE connected; discovering MIDI service")
        peripheral.discoverServices([serviceID])
    }
    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        lost(error)
    }
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        lost(error)
    }
    func clear() {
        for note in heldNotes { publish([0x8F, note, 0]) }
        heldNotes.removeAll()
        feedbackDecoder.reset()
        lastLoggedState = nil
        ready = false; characteristic = nil; writes.removeAll(); writing = false
    }
    func lost(_ error: Error?) {
        clear()
        print("BLE lost: \(String(describing: error)); retry in 1 second")
        retry?.invalidate()
        retry = Timer.scheduledTimer(withTimeInterval: 1, repeats: false) { [weak self] _ in self?.connect() }
    }
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard error == nil, let service = peripheral.services?.first(where: { $0.uuid == serviceID }) else {
            central.cancelPeripheralConnection(peripheral); return
        }
        peripheral.discoverCharacteristics([characteristicID], for: service)
    }
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard error == nil, let found = service.characteristics?.first(where: { $0.uuid == characteristicID }),
              found.properties.contains(.notify),
              found.properties.contains(.write) || found.properties.contains(.writeWithoutResponse) else {
            central.cancelPeripheralConnection(peripheral); return
        }
        characteristic = found
        print("MIDI characteristic properties: \(found.properties.rawValue)")
        peripheral.setNotifyValue(true, for: found)
    }
    func peripheral(_ peripheral: CBPeripheral, didUpdateNotificationStateFor characteristic: CBCharacteristic, error: Error?) {
        guard characteristic === self.characteristic else { return }
        ready = error == nil && characteristic.isNotifying
        print("MIDI notifications ready: \(ready)")
        if !ready { central.cancelPeripheralConnection(peripheral) }
    }
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard ready, characteristic === self.characteristic, error == nil, let data = characteristic.value else { return }
        do {
            for message in try BLEChannelCodec.decode(Array(data)) {
                guard ControllerRoute.fromController(message) else { continue }
                if [UInt8(0x9F), 0x8F].contains(message[0]) && [UInt8(60), 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75].contains(message[1]) {
                    if message[0] == 0x9F && message[2] > 0 { heldNotes.insert(message[1]) }
                    else { heldNotes.remove(message[1]) }
                }
                print("RX \(message.map { String(format: "%02X", $0) }.joined(separator: " "))")
                publish(message)
            }
        } catch { print("Dropped unsupported/malformed BLE packet") }
    }
    func publish(_ message: [UInt8]) {
        var list = MIDIPacketList()
        withUnsafeMutablePointer(to: &list) { pointer in
            let packet = MIDIPacketListInit(pointer)
            _ = message.withUnsafeBufferPointer { MIDIPacketListAdd(pointer, MemoryLayout<MIDIPacketList>.size, packet, 0, $0.count, $0.baseAddress!) }
            MIDIReceived(source, pointer)
        }
    }
    func feedback(_ message: [UInt8]) {
        // CoreMIDI may batch several messages into one packet.
        guard ready, ControllerRoute.toController(message) else { return }
        guard let encoded = try? BLEChannelCodec.encode(message, milliseconds: UInt64(ProcessInfo.processInfo.systemUptime * 1000)) else { return }
        // Bound traffic. Never queue or replay commands while disconnected.
        guard writes.count < 64 else { print("Feedback queue full; dropped update"); return }
        writes.append(encoded)
        if message[1] == 119 && lastLoggedState != message[2] {
            lastLoggedState = message[2]
            print("TX state \(message[2])")
        }
        pump()
    }
    func pump() {
        guard ready, !writing, !writes.isEmpty, let peripheral = peripheral, let characteristic = characteristic else { return }
        if characteristic.properties.contains(.write) {
            writing = true
            peripheral.writeValue(BLEWriteBatch.take(&writes, maximum: peripheral.maximumWriteValueLength(for: .withResponse)), for: characteristic, type: .withResponse)
        } else {
            while !writes.isEmpty && peripheral.canSendWriteWithoutResponse {
                peripheral.writeValue(BLEWriteBatch.take(&writes, maximum: peripheral.maximumWriteValueLength(for: .withoutResponse)), for: characteristic, type: .withoutResponse)
            }
        }
    }
    func peripheralIsReady(toSendWriteWithoutResponse peripheral: CBPeripheral) {
        pump()
    }
    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        guard characteristic === self.characteristic else { return }
        writing = false
        if let error = error { print("Write failed: \(error)"); central.cancelPeripheralConnection(peripheral) }
        else { pump() }
    }
}

@main struct BridgeMain {
    static func main() {
        guard CommandLine.arguments.count == 2, let target = UUID(uuidString: CommandLine.arguments[1]) else {
            print("Usage: mac_reconnect_bridge <selected peripheral UUID>"); exit(2)
        }
        let bridge = ReconnectBridge(target: target)
        withExtendedLifetime(bridge) { RunLoop.main.run() }
    }
}
