import Foundation

enum BLECodecError: Error { case malformed, unsupported }

struct ChannelMIDIStream {
    var status: UInt8 = 0
    var pending: [UInt8] = []
    var sysEx = false
    mutating func reset() { status = 0; pending = []; sysEx = false }
    mutating func feed(_ bytes: [UInt8]) -> [[UInt8]] {
        var messages: [[UInt8]] = []
        for byte in bytes {
            if byte >= 0xF8 { continue }
            if byte & 0x80 != 0 {
                pending = []
                if byte == 0xF0 { sysEx = true; status = 0 }
                else if byte == 0xF7 { sysEx = false; status = 0 }
                else if byte < 0xF0 { sysEx = false; status = byte }
                else { status = 0 }
            } else if !sysEx && status != 0 {
                pending.append(byte)
                let kind = status & 0xF0
                if pending.count == ((kind == 0xC0 || kind == 0xD0) ? 1 : 2) {
                    messages.append([status] + pending)
                    pending = []
                }
            }
        }
        return messages
    }
}

// Controller-only codec. Channel messages are complete within each BLE packet;
// timestamps and running status may be shared. SysEx/system-common are rejected.
enum BLEChannelCodec {
    static func decode(_ packet: [UInt8]) throws -> [[UInt8]] {
        guard packet.count >= 3, packet[0] & 0xC0 == 0x80, packet[1] & 0x80 != 0 else {
            throw BLECodecError.malformed
        }
        var status: UInt8 = 0
        var data: [UInt8] = []
        var highIsStatus = true
        var messages: [[UInt8]] = []
        for byte in packet.dropFirst(2) {
            if byte & 0x80 != 0 {
                if !highIsStatus { highIsStatus = true; continue } // timestamp low
                highIsStatus = false
                if byte >= 0xF8 { continue } // realtime does not cancel running status
                guard byte < 0xF0 else { throw BLECodecError.unsupported }
                guard data.isEmpty else { throw BLECodecError.malformed }
                status = byte
            } else {
                highIsStatus = false
                guard status >= 0x80 else { throw BLECodecError.malformed }
                data.append(byte)
                let kind = status & 0xF0
                let length = (kind == 0xC0 || kind == 0xD0) ? 1 : 2
                if data.count == length {
                    messages.append([status] + data)
                    data.removeAll(keepingCapacity: true)
                }
            }
        }
        guard data.isEmpty else { throw BLECodecError.malformed }
        return messages
    }
    static func encode(_ message: [UInt8], milliseconds: UInt64) throws -> Data {
        guard let status = message.first, status >= 0x80, status < 0xF0 else {
            throw BLECodecError.unsupported
        }
        let kind = status & 0xF0
        let length = (kind == 0xC0 || kind == 0xD0) ? 2 : 3
        guard message.count == length, message.dropFirst().allSatisfy({ $0 < 0x80 }) else {
            throw BLECodecError.malformed
        }
        let time = milliseconds & 0x1FFF
        return Data([0x80 | UInt8(time >> 7), 0x80 | UInt8(time & 0x7F)] + message)
    }
}

// Only the negotiated controller protocol may cross the optional Mac bridge.
enum ControllerRoute {
    static func fromController(_ message: [UInt8]) -> Bool {
        guard message.count == 3, message.dropFirst().allSatisfy({ $0 < 128 }) else { return false }
        if [UInt8(0x9F), 0x8F].contains(message[0]) {
            return [UInt8(60), 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75].contains(message[1])
        }
        return message[0] == 0xBF && [UInt8(20), 21, 89, 90, 91, 92, 93, 104, 105, 106, 107, 108, 109, 111, 113, 117].contains(message[1])
    }
    static func toController(_ message: [UInt8]) -> Bool {
        return message.count == 3 && message[0] == 0xBF && message.dropFirst().allSatisfy({ $0 < 128 }) &&
            [UInt8(20), 22, 90, 91, 92, 93, 110, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 112, 114, 116, 117, 118, 119].contains(message[1])
    }
}

// Preserve message order and timestamp-high boundaries while sharing a GATT
// write. Each queued element is one complete packet produced by encode().
enum BLEWriteBatch {
    static func take(_ queue: inout [Data], maximum: Int) -> Data {
        precondition(!queue.isEmpty && maximum >= queue[0].count)
        var packet = queue.removeFirst()
        while let next = queue.first, next.first == packet.first,
              packet.count + next.count - 1 <= maximum {
            packet.append(contentsOf: next.dropFirst())
            queue.removeFirst()
        }
        return packet
    }
}
