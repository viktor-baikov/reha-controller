import Foundation
import CoreMIDI

@main struct SimulatorMain {
    static func main() {
        let wanted = CommandLine.arguments.count > 1 ? CommandLine.arguments[1] : "Reha Probe Bridge"
        var client = MIDIClientRef(), input = MIDIPortRef(), output = MIDIPortRef()
        var source = MIDIEndpointRef(), destination = MIDIEndpointRef()
        var decoder = ChannelMIDIStream()
        var model = RemoteSimulator()
        var heartbeat: UInt8 = 0
        var textQueue: [(UInt8,UInt8)] = []
        var lastRows: [String] = []
        func name(_ object: MIDIObjectRef) -> String {
            var value: Unmanaged<CFString>?
            MIDIObjectGetStringProperty(object, kMIDIPropertyDisplayName, &value)
            return value?.takeRetainedValue() as String? ?? ""
        }
        func check(_ status: OSStatus) { precondition(status == noErr, "CoreMIDI error \(status)") }
        func send(_ controller: UInt8, _ value: UInt8) {
            guard destination != 0 else { return }
            var list = MIDIPacketList()
            let bytes: [UInt8] = [0xBF, controller, value]
            withUnsafeMutablePointer(to: &list) { pointer in
                let packet = MIDIPacketListInit(pointer)
                _ = bytes.withUnsafeBufferPointer { MIDIPacketListAdd(pointer, MemoryLayout<MIDIPacketList>.size, packet, 0, $0.count, $0.baseAddress!) }
                MIDISend(output, destination, pointer)
            }
        }
        func snapshot() {
            guard model.handshake else { return }
            send(RemoteProtocol.hello, RemoteProtocol.version)
            send(112,31)
            send(20,UInt8(model.volume)); send(22,model.muted ? 127 : 0)
            send(RemoteProtocol.state, model.state.rawValue)
            send(RemoteProtocol.heartbeat, heartbeat)
            heartbeat = (heartbeat + 1) & 127
            let rows = ["T\(model.take) Song \(model.song) [SIM]", "\(model.state) \(model.position)s".uppercased()]
            if textQueue.isEmpty && rows != lastRows {
                lastRows=rows
                for (row,value) in rows.enumerated() {
                    let chars=Array(value.utf8.prefix(32));let sum=(row+chars.count+chars.reduce(0){$0+Int($1)})&127
                    textQueue += [(100,UInt8(row)),(101,UInt8(chars.count))]
                    textQueue += chars.map{(102,$0)};textQueue.append((103,UInt8(sum)))
                }
            }
        }
        func receive(_ bytes: [UInt8]) {
            for message in decoder.feed(bytes) {
                guard message[0] & 15 == 15 else { continue }
                if message[0] == 0xBF && message[1] == RemoteProtocol.hello {
                    textQueue.removeAll();lastRows=[]
                    model.connectionChanged() // firmware announces a new link epoch
                    model.handshake = message[2] == RemoteProtocol.version
                    snapshot()
                } else if message[0] == 0xBF && message[1] == RemoteProtocol.token {
                    model.token = message[2]
                } else if message[0] == 0xBF && [UInt8(20),21].contains(message[1]) && model.handshake {
                    model.volume = message[1]==20 ? Int(message[2]) : max(0,min(127,model.volume+Int(message[2])-64))
                    print("SIMULATED volume=\(model.volume)/127 muted=\(model.muted)"); snapshot()
                } else if message.count == 3 && [UInt8(0x8F), 0x9F].contains(message[0]) {
                    let note = message[1]
                    if message[0] == 0x8F || message[2] == 0 { model.held.remove(note); continue }
                    let previousState = model.state
                    guard let token = model.token, let result = model.command(note) else { continue }
                    send(RemoteProtocol.ackToken, token)
                    send(RemoteProtocol.result, result.rawValue)
                    snapshot()
                    print("SIMULATED command=\(note) token=\(token) result=\(result) state=\(model.state) muted=\(model.muted) song=\(model.song) take=\(model.take)")
                    if previousState != .saving && model.state == .saving && result == .accepted {
                        DispatchQueue.main.asyncAfter(deadline: .now() + 0.4) {
                            model.finishSave(); snapshot()
                            print("SIMULATED take kept; total=\(model.takes)")
                        }
                    }
                }
            }
        }
        check(MIDIClientCreateWithBlock("Reha controller simulator" as CFString, &client) { _ in })
        check(MIDIOutputPortCreate(client, "State" as CFString, &output))
        check(MIDIInputPortCreateWithBlock(client, "Commands" as CFString, &input) { list, _ in
            let offset = MemoryLayout<MIDIPacketList>.offset(of: \MIDIPacketList.packet)!
            var packet = UnsafeRawPointer(list).advanced(by: offset).assumingMemoryBound(to: MIDIPacket.self)
            var bytes: [UInt8] = []
            for _ in 0..<list.pointee.numPackets {
                let offset = MemoryLayout<MIDIPacket>.offset(of: \MIDIPacket.data)!
                let data = UnsafeRawPointer(packet).advanced(by: offset).assumingMemoryBound(to: UInt8.self)
                bytes.append(contentsOf: UnsafeBufferPointer(start: data, count: Int(packet.pointee.length)))
                packet = UnsafePointer(MIDIPacketNext(packet))
            }
            DispatchQueue.main.async { receive(bytes) }
        })
        func refresh() {
            let ins = (0..<MIDIGetNumberOfSources()).map { MIDIGetSource($0) }.filter { name($0) == wanted }
            let outs = (0..<MIDIGetNumberOfDestinations()).map { MIDIGetDestination($0) }.filter { name($0) == wanted }
            let nextIn = ins.count == 1 ? ins[0] : 0
            let nextOut = outs.count == 1 ? outs[0] : 0
            if source != nextIn || destination != nextOut {
                if source != 0 { MIDIPortDisconnectSource(input, source) }
                source = nextIn; destination = nextOut
                model.connectionChanged(); decoder.reset();textQueue.removeAll();lastRows=[]
                if source != 0 { check(MIDIPortConnectSource(input, source, nil)) }
                print("Port: \(source != 0 && destination != 0 ? wanted : "waiting")")
            }
            snapshot()
        }
        print("SIMULATOR ONLY — no audio recorded. Waiting for protocol v1 on \(wanted)")
        refresh()
        let timer = Timer.scheduledTimer(withTimeInterval: 1, repeats: true) { _ in
            DispatchQueue.main.async { refresh() }
        }
        let textTimer=Timer.scheduledTimer(withTimeInterval:0.05,repeats:true) { _ in
            DispatchQueue.main.async {
                for _ in 0..<min(4,textQueue.count) {let message=textQueue.removeFirst();send(message.0,message.1)}
            }
        }
        withExtendedLifetime((timer,textTimer)) { RunLoop.main.run() }
    }
}
