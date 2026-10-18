import AppKit
import CoreAudioKit
import CoreMIDI

// Apple's Bluetooth MIDI picker, hosted inside our own diagnostic application.
// Opens the system picker without launching Audio MIDI Setup.
final class SetupDelegate: NSObject, NSApplicationDelegate {
    var client = MIDIClientRef()
    var controller: CABTLEMIDIWindowController?
    func applicationDidFinishLaunching(_ notification: Notification) {
        let status = MIDIClientCreate("Reha MIDI Setup" as CFString, nil, nil, &client)
        guard status == noErr else { fatalError("CoreMIDI initialization: \(status)") }
        controller = CABTLEMIDIWindowController()
        controller?.showWindow(nil)
        NSApp.activate(ignoringOtherApps: true)
    }
    func applicationWillTerminate(_ notification: Notification) {
        MIDIClientDispose(client)
    }
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }
}
let app = NSApplication.shared
let delegate = SetupDelegate()
app.delegate = delegate
app.setActivationPolicy(.regular)
app.run()
