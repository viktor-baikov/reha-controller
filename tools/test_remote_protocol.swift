import Foundation
@main struct ProtocolTests {
    static func main() {
        var model = RemoteSimulator()
        model.token = 1
        precondition(model.command(60) == nil)
        model.handshake = true
        model.token = 1
        precondition(model.command(60) == .accepted && model.state == .recording)
        model.token = 2
        precondition(model.command(60) == nil) // held key
        model.held.remove(60)
        model.token = 1
        precondition(model.command(60) == .accepted && model.state == .recording) // cached token
        model.held.remove(60)
        model.token = 2
        precondition(model.command(62) == .busy && model.state == .recording)
        model.held.removeAll()
        model.token = 3
        precondition(model.command(60) == .accepted && model.state == .saving)
        model.held.removeAll()
        model.token = 4
        precondition(model.command(60) == .busy)
        model.finishSave(); model.finishSave()
        precondition(model.takes == 1 && model.state == .ready)
        model.held.removeAll(); model.token = 5
        precondition(model.command(62) == .accepted && model.state == .playing)
        model.held.removeAll(); model.token = 6
        precondition(model.command(62) == .accepted && model.state == .ready)
        model.connectionChanged()
        precondition(!model.handshake && model.lastCommand == nil && model.takes == 1)
        model.handshake = true; model.token = 1
        precondition(model.command(60) == .accepted && model.state == .recording)
        var empty = RemoteSimulator(); empty.handshake = true; empty.token = 1
        precondition(empty.command(62) == .unavailable)
        var panel=RemoteSimulator();panel.handshake=true;panel.token=1
        precondition(panel.command(70) == .accepted && panel.muted)
        panel.held.removeAll();panel.token=1
        precondition(panel.command(70) == .accepted && panel.muted) // token dedup
        panel.held.removeAll();panel.token=2
        precondition(panel.command(69) == .accepted && panel.song==2)
        panel.held.removeAll();panel.token=3
        precondition(panel.command(67) == .accepted && panel.position==1)
        print("Controller protocol simulator tests passed")
    }
}
