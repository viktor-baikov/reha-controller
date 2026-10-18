import Foundation

enum RemoteProtocol {
    static let version: UInt8 = 1
    static let token: UInt8 = 113, ackToken: UInt8 = 114, result: UInt8 = 116
    static let hello: UInt8 = 117, heartbeat: UInt8 = 118, state: UInt8 = 119
    static let record: UInt8 = 60, play: UInt8 = 62
}
enum RemoteState: UInt8 { case ready = 0, recording, playing, saving, error, unavailable }
enum CommandResult: UInt8 { case accepted = 1, busy, unavailable, error }

// Diagnostic simulator only. The real recorder will supply its own operations.
struct RemoteSimulator {
    var state = RemoteState.ready
    var takes = 0
    var volume = 64, song = 1, take = 1, position = 0
    var muted = false
    var handshake = false
    var token: UInt8?
    var lastCommand: (token: UInt8, note: UInt8, result: CommandResult)?
    var held = Set<UInt8>()
    mutating func connectionChanged() {
        handshake = false; token = nil; lastCommand = nil; held.removeAll()
    }
    mutating func command(_ note: UInt8) -> CommandResult? {
        guard handshake, let token = token, !held.contains(note) else { return nil }
        self.token = nil
        held.insert(note)
        if let last = lastCommand, last.token == token {
            return last.note == note ? last.result : .error
        }
        let result: CommandResult
        if state == .saving { result = .busy }
        else if state == .error || state == .unavailable { result = .unavailable }
        else if note == RemoteProtocol.record {
            state = state == .recording ? .saving : .recording
            result = .accepted
        } else if note == RemoteProtocol.play {
            if state == .recording { result = .busy }
            else if state == .playing { state = .ready; result = .accepted }
            else if takes == 0 { result = .unavailable }
            else { state = .playing; result = .accepted }
        } else if note == 70 {
            muted.toggle(); result = .accepted
        } else if (63...69).contains(note) {
            if state == .recording { result = .busy }
            else {
                switch note {
                    case 63: position = 0
                    case 65: position = max(0,position-1)
                    case 67: position += 1
                    case 64: take = max(1,take-1)
                    case 66: take += 1
                    case 68: song = max(1,song-1); position=0; state = .ready
                    case 69: song += 1; position=0; state = .ready
                    default: break
                }
                result = .accepted
            }
        } else { result = .error }
        lastCommand = (token, note, result)
        return result
    }
    mutating func finishSave() {
        if state == .saving { takes += 1; state = .ready }
    }
}
