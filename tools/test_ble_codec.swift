import Foundation

@main struct CodecTests {
    static func main() throws {
        func expect(_ packet: [UInt8], _ messages: [[UInt8]]) throws {
            let decoded = try BLEChannelCodec.decode(packet)
            precondition(decoded == messages)
        }
        try expect([0x80, 0x80, 0x9F, 60, 127], [[0x9F, 60, 127]])
        try expect([0x80, 0x80, 0x9F, 60, 127, 61, 1], [[0x9F, 60, 127], [0x9F, 61, 1]])
        try expect([0x80, 0x80, 0x9F, 60, 127, 0x81, 60, 0], [[0x9F, 60, 127], [0x9F, 60, 0]])
        try expect([0x80, 0x80, 0x9F, 60, 127, 0x81, 0x8F, 60, 0], [[0x9F, 60, 127], [0x8F, 60, 0]])
        try expect([0xBF, 0xFF, 0xCF, 7, 0x80, 0xBF, 119, 1], [[0xCF, 7], [0xBF, 119, 1]])
        try expect([0x80, 0x80, 0x9F, 60, 0x81, 0xF8, 127], [[0x9F, 60, 127]])
        for time: UInt64 in [0, 127, 128, 8191, 8192] {
            let bytes: [UInt8] = [0xBF, 119, 1]
            let decoded = try BLEChannelCodec.decode(Array(BLEChannelCodec.encode(bytes, milliseconds: time)))
            precondition(decoded == [bytes])
        }
        for packet: [UInt8] in [[], [0x80], [0, 0x80, 0x9F, 60, 127], [0x80, 60, 127],
                               [0x80, 0x80, 60, 127], [0x80, 0x80, 0x9F, 60],
                               [0x80, 0x80, 0x9F, 60, 127, 61],
                               [0x80, 0x80, 0xF0, 1, 0x80, 0xF7]] {
            do { _ = try BLEChannelCodec.decode(packet); fatalError("Accepted invalid packet: \(packet)") }
            catch is BLECodecError {}
        }
        for message: [UInt8] in [[], [0xF0], [0xBF, 119], [0xBF, 119, 128]] {
            do { _ = try BLEChannelCodec.encode(message, milliseconds: 0); fatalError("Accepted invalid MIDI") }
            catch is BLECodecError {}
        }
        for limit in [20, 64, 185] {
            let messages: [[UInt8]] = (0..<60).map { [0xBF, UInt8(100 + $0 % 10), UInt8($0)] }
            var queue = try messages.enumerated().map { try BLEChannelCodec.encode($0.element, milliseconds: UInt64(120 + $0.offset)) }
            var decoded: [[UInt8]] = []
            while !queue.isEmpty {
                let packet = BLEWriteBatch.take(&queue, maximum: limit)
                precondition(packet.count <= limit)
                decoded += try BLEChannelCodec.decode(Array(packet))
            }
            precondition(decoded == messages)
        }
        for control: UInt8 in 104...109 {
            precondition(ControllerRoute.fromController([0xBF, control, 1]))
            precondition(ControllerRoute.toController([0xBF, control, 1]))
        }
        precondition(ControllerRoute.fromController([0x9F, 73, 127]))
        precondition(!ControllerRoute.fromController([0x9E, 73, 127]))
        precondition(!ControllerRoute.toController([0xBF, 99, 0]))
        precondition(!ControllerRoute.toController([0xBF, 119, 128]))
        precondition(!ControllerRoute.toController([0xBF, 119]))
        for cc: UInt8 in 90...93 {
            precondition(ControllerRoute.fromController([0xBF,cc,1]))
            precondition(ControllerRoute.toController([0xBF,cc,1]))
        }
        precondition(ControllerRoute.fromController([0xBF,89,1]))
        precondition(ControllerRoute.toController([0xBF,110,1]))
        for note: UInt8 in 74...75 {precondition(ControllerRoute.fromController([0x9F,note,127]))}
        var stream = ChannelMIDIStream()
        precondition(stream.feed([0xBF, 118, 1, 0xBF, 119, 0]) == [[0xBF, 118, 1], [0xBF, 119, 0]])
        precondition(stream.feed([0xBF, 118]).isEmpty)
        precondition(stream.feed([0xF8, 1, 119, 1]) == [[0xBF, 118, 1], [0xBF, 119, 1]])
        stream.reset()
        precondition(stream.feed([1, 119, 1]).isEmpty)
        precondition(stream.feed([0xF0, 1, 2, 0xF7, 0xBF, 119, 0]) == [[0xBF, 119, 0]])
        print("BLE channel codec and MIDI stream tests passed")
    }
}
