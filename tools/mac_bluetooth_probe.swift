import Foundation
import CoreBluetooth
import CoreMIDI

// Diagnostic fallback: discover/connect only our MIDI probe, then let Apple's
// MIDI driver handle the protocol. No custom BLE MIDI parser is introduced.
final class BluetoothProbe: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    var midiClient = MIDIClientRef()
    var manager: CBCentralManager!
    var selected: CBPeripheral?
    var activationTimer: Timer?
    var activationAttempts = 0
    var handedOff = false
    let connect = CommandLine.arguments.contains("--connect")
    let service = CBUUID(string: "03B80E5A-EDE8-4B33-A751-6CE34EC4C700")
    let characteristic = CBUUID(string: "7772E5DB-3868-4112-A1A9-F2669D106BF3")

    override init() {
        super.init()
        let status = MIDIClientCreateWithBlock("Reha Bluetooth connection" as CFString, &midiClient) { _ in }
        guard status == noErr else { fatalError("CoreMIDI client: \(status)") }
        manager = CBCentralManager(delegate: self, queue: .main)
    }
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        print("Bluetooth state: \(central.state.rawValue); authorization: \(CBManager.authorization.rawValue)")
        if central.state == .poweredOn {
            central.scanForPeripherals(withServices: [service], options: nil)
            print("Scanning BLE MIDI; connect mode: \(connect)")
        } else if central.state == .unauthorized {
            print("Bluetooth access denied. No pairing performed.")
            exit(2)
        }
    }
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi RSSI: NSNumber) {
        let name = advertisementData[CBAdvertisementDataLocalNameKey] as? String ?? peripheral.name ?? "<unnamed>"
        print("BLE MIDI: \(name), id=\(peripheral.identifier), RSSI=\(RSSI)")
        guard connect, name == "Reha Probe", selected == nil else { return }
        selected = peripheral
        peripheral.delegate = self
        central.stopScan()
        central.connect(peripheral, options: nil)
    }
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        handedOff = false
        print("Connected; discovering MIDI service")
        peripheral.discoverServices([service])
    }
    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        print("Connect failed: \(String(describing: error))")
    }
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        print("Disconnected: \(String(describing: error))")
        activationTimer?.invalidate()
        if !handedOff && connect {
            print("Waiting to reconnect to selected peripheral")
            central.connect(peripheral, options: nil)
        }
    }
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard error == nil, let found = peripheral.services?.first(where: { $0.uuid == service }) else {
            print("Service discovery failed: \(String(describing: error))"); return
        }
        peripheral.discoverCharacteristics([characteristic], for: found)
    }
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard error == nil, let found = service.characteristics?.first(where: { $0.uuid == characteristic }) else {
            print("Characteristic discovery failed: \(String(describing: error))"); return
        }
        // Reading the encrypted characteristic lets the OS negotiate pairing.
        // CoreMIDI activation is requested only once this succeeds.
        peripheral.readValue(for: found)
    }
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard error == nil else { print("Encrypted read failed: \(String(describing: error))"); return }
        activationAttempts = 0
        activate()
        if !handedOff {
            activationTimer = Timer.scheduledTimer(withTimeInterval: 1, repeats: true) { [weak self] timer in
                guard let self = self else { timer.invalidate(); return }
                self.activate()
                if self.handedOff || self.activationAttempts >= 10 { timer.invalidate() }
            }
        }
    }
    func activate() {
        activationAttempts += 1
        let status = MIDIBluetoothDriverActivateAllConnections()
        print("CoreMIDI activation \(activationAttempts): status=\(status), devices=\(MIDIGetNumberOfDevices()), inputs=\(MIDIGetNumberOfSources()), outputs=\(MIDIGetNumberOfDestinations())")
        for i in 0..<MIDIGetNumberOfDevices() {
            let device = MIDIGetDevice(i)
            var name: Unmanaged<CFString>?
            MIDIObjectGetStringProperty(device, kMIDIPropertyName, &name)
            let text = name?.takeRetainedValue() as String? ?? "<unnamed>"
            guard text.contains("Reha Probe") else { continue }
            var offline: Int32 = 1
            MIDIObjectGetIntegerProperty(device, kMIDIPropertyOffline, &offline)
            print("Matched MIDI device: \(text), offline=\(offline)")
            if offline == 0, let selected = selected {
                handedOff = true
                manager.cancelPeripheralConnection(selected)
                print("Handed connection to CoreMIDI")
            }
        }
    }
}

print("Reha Bluetooth diagnostic: discovery unless --connect is supplied. Ctrl-C exits.")
let probe = BluetoothProbe()
RunLoop.main.run()
