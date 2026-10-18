"""SSH-stdio recorder probe: real MIDI, fake audio, disposable settings/library.

Developer tool, not an application service. Requires a recorder source checkout
with its existing test dependencies. EOF closes MIDI and removes temporary data.
Only JSON requests on stdin are accepted; stdout carries JSON responses.
"""
import argparse
import importlib
import json
from pathlib import Path
import sys
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--recorder', type=Path, required=True)
    parser.add_argument('--controller', default='Reha Probe')
    args = parser.parse_args()
    root = args.recorder.resolve()
    sys.path[:0] = [str(root / 'src'), str(root / 'tests')]
    protocol = sys.stdout
    # App/test/library diagnostic output must not corrupt the JSON channel.
    sys.stdout = sys.stderr
    from test_engine import fresh_api, np, SR
    from rehearsal_recorder.midi.ports import open_system, open_controller_system
    from rehearsal_recorder.midi.rig import MidiRig
    from rehearsal_recorder.midi.controller import Controller

    def reply(value):
        protocol.write(json.dumps(value, default=str, ensure_ascii=True) + '\n')
        protocol.flush()

    with tempfile.TemporaryDirectory(prefix='reha-remote-probe-') as temporary:
        _, api = fresh_api(Path(temporary))
        system = None
        try:
            sys.modules.pop('pylibremidi', None)
            importlib.invalidate_caches()
            system, error = open_system(quiet=True)
            if system is None:
                raise RuntimeError(error)
            api._controller.close()
            api._midi.shutdown()
            controller_system = open_controller_system(system)
            api._midi_system = system
            api._controller_system = controller_system
            api._midi = MidiRig(system, threads=False)
            api._controller = Controller(controller_system, api._transport, threads=True)
            api._config['device'] = 0
            api.save_default_tracks({'device_index': 0, 'samplerate': SR,
                                     'tracks': [{'name': 'Test', 'channel': 1}]})
            settings = api.get_settings
            api.get_settings = lambda: {**settings(), 'device_index': 0,
                                       'missing_device': None, 'samplerate': SR}

            def configure():
                end = time.monotonic() + 8
                while time.monotonic() < end:
                    inputs = [p for p in controller_system.inputs() if args.controller in p.name]
                    outputs = [p for p in controller_system.outputs() if args.controller in p.name]
                    if len(inputs) == len(outputs) == 1:
                        result = api.set_controller(inputs[0].saved(), outputs[0].saved())
                        if not result.get('ok'):
                            raise RuntimeError(result)
                        return result
                    time.sleep(.1)
                raise RuntimeError('Controller input/output missing or ambiguous')

            configure()

            def status():
                transport = api._transport.controller_state()
                transport.pop('selection_reference', None)
                return {'backend': controller_system.name, 'instrument_backend': system.name, 'transport': transport,
                        'controller': api.controller_state(), 'player': api.player_state(),
                        'takes': [{'number': t['take_number'], 'song': t.get('song')}
                                  for t in api._session_takes()] if api._session else []}

            reply({'event': 'ready', 'isolated': True, **status()})
            for line in sys.stdin:
                try:
                    request = json.loads(line)
                    op = request.get('op')
                    if op == 'status':
                        result = status()
                    elif op == 'start':
                        result = api.start_rehearsal('Remote controller check', 0, SR,
                                                    [{'name': 'Test', 'channel': 1}])
                        if result.get('ok'):
                            api._session['set'] = {'songs': ['One', 'Empty A', 'Empty B']}
                            api.set_next_take_name('One')
                    elif op == 'fill':
                        if api._recorder is None:
                            raise RuntimeError('No active test recording')
                        # Five seconds is long enough to inspect playback/seek.
                        frames = SR * 5
                        api._recorder._callback(np.full((frames, 1), 1000, dtype=np.int32),
                                               frames, None, None)
                        result = {'ok': True}
                    elif op == 'controller_off':
                        result = api.set_controller(None)
                    elif op == 'controller_on':
                        result = configure()
                    elif op == 'undo':
                        undo = (api._controller.last_action or {}).get('undo')
                        if not undo:
                            raise RuntimeError('No controller removal to undo')
                        result = api.undo_trash(undo)
                    elif op == 'action':
                        # Host-state setup/recovery checks only. Physical BLE
                        # actions are independently driven over the board UART.
                        action = request.get('action')
                        if action not in {'restart', 'stop_rehearsal', 'start_rehearsal',
                                          'previous_song', 'next_song', 'previous_take',
                                          'next_take', 'record_stop', 'play_stop'}:
                            raise ValueError('Unsupported test action')
                        result = api._transport.command(action)
                    elif op == 'quit':
                        reply({'ok': True, 'result': 'closing'})
                        break
                    else:
                        raise ValueError('Unsupported probe operation')
                    reply({'ok': True, 'result': result})
                except Exception as error:
                    reply({'ok': False, 'error': str(error)})
        finally:
            api.shutdown()
            if system is not None:
                system.close()


if __name__ == '__main__':
    main()
