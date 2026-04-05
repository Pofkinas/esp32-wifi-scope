import queue
import threading
import tkinter as tk
from tkinter import ttk, messagebox
from dataclasses import dataclass

import numpy as np
import serial
import serial.tools.list_ports

from frame_reader import SerialFrameReader
from scope_plot import ScopePlot


@dataclass
class Config:
    ADC_SAMPLE_HZ:      int   = 60000       # 60 KSps
    DEFAULT_BAUD:       int   = 1000000
    TIMEBASE_MIN:       float = 1e-6        # 1 µs/div
    TIMEBASE_MAX:       float = 5.0         # 5 s/div
    VOLTAGE_MIN:        float = 0.0
    VOLTAGE_MAX:        float = 3300.0
    VOLTAGE_SLIDER_MAX: float = 5000.0
    BUFFER_MAX:         int   = 10485760    # 10 MB
    CMD_SCOPE_START:    str   = 'scope_start'
    CMD_SCOPE_STOP:     str   = 'scope_stop'


class SampleBuffer:
    def __init__(self, max_size):
        self._data  = np.zeros(max_size, dtype=np.uint16)
        self._count = 0
        self.target = 1000

    def push(self, samples):
        space = self.target - self._count
        take  = max(0, min(len(samples), space))

        self._data[self._count:self._count + take] = samples[:take]
        self._count += take

        return self._count >= self.target

    def view(self):
        return self._data[:self._count]

    def reset(self):
        self._count = 0


class OscilloscopeApp:
    def __init__(self, root, cfg):
        self._root   = root
        self._cfg    = cfg
        self._queue  = queue.Queue()
        self._reader = SerialFrameReader(debug=lambda msg: self._queue.put(('debug', msg)))
        self._buffer = SampleBuffer(cfg.BUFFER_MAX)

        self._paused        = False
        self._scope_running = False
        self._tb_seconds    = 0.05
        self._tb_unit       = 'ms'
        self._stop          = threading.Event()

        root.title("ESP32 Oscilloscope")
        root.configure(bg='#1e1e1e')

        self._plot = ScopePlot(root, cfg, on_cursor_move=self._cursor_moved, on_delta_move=self._delta_moved)

        self._init_controls()
        self._init_debug_panel()

        self._root.after(20, self._queue_poll)

    def _init_controls(self):
        lbl = dict(bg='#2a2a2a', fg='white')
        ent = dict(bg='#1e1e1e', fg='white', relief=tk.FLAT, insertbackground='white')
        btn = dict(bg='#3a3a3a', fg='white', relief=tk.FLAT, padx=8)

        ctrl = tk.Frame(self._root, bg='#2a2a2a')
        ctrl.pack(fill=tk.X, padx=4, pady=2)

        tk.Label(ctrl, text="Timebase:", **lbl).pack(side=tk.LEFT, padx=(4, 2))

        self._tb_slider = ttk.Scale(ctrl, from_=0, to=1000, orient=tk.HORIZONTAL, length=180, command=self._timebase_changed)
        self._tb_slider.pack(side=tk.LEFT, padx=4)

        self._tb_entry_var = tk.StringVar()
        tb_entry = tk.Entry(ctrl, textvariable=self._tb_entry_var, width=6, **ent)
        tb_entry.bind('<Return>', self._timebase_entry_apply)
        tb_entry.pack(side=tk.LEFT, padx=2)

        self._tb_unit_lbl = tk.Label(ctrl, text="ms", width=3, **lbl)
        self._tb_unit_lbl.pack(side=tk.LEFT, padx=(0, 8))

        self._tb_slider.set(700)
        self._timebase_changed(700)

        self._scope_btn = tk.Button(ctrl, text="Start", command=self._scope_toggle, bg='#1a4a1a', fg='#00e676', relief=tk.FLAT, padx=8)
        self._scope_btn.pack(side=tk.LEFT, padx=8)

        self._pause_btn = tk.Button(ctrl, text="Pause", command=self._pause_toggle, **btn)
        self._pause_btn.pack(side=tk.LEFT, padx=4)

        self._cursor_btn = tk.Button(ctrl, text="Cursor", command=self._cursor_toggle, **btn)
        self._cursor_btn.pack(side=tk.LEFT, padx=4)

        self._cursor_readout = tk.Label(ctrl, text="", width=10, bg='#2a2a2a', fg='#ffff00', font=('Courier', 9))
        self._cursor_readout.pack(side=tk.LEFT, padx=(0, 4))

        self._delta_btn = tk.Button(ctrl, text="ΔT", command=self._delta_toggle, **btn)
        self._delta_btn.pack(side=tk.LEFT, padx=4)

        self._delta_readout = tk.Label(ctrl, text="", width=10, bg='#2a2a2a', fg='#ff9800', font=('Courier', 9), justify='left')
        self._delta_readout.pack(side=tk.LEFT, padx=(0, 8))

        tk.Label(ctrl, text="Port:", **lbl).pack(side=tk.LEFT, padx=(16, 2))

        self._port_var   = tk.StringVar()
        self._port_combo = ttk.Combobox(ctrl, textvariable=self._port_var, width=10)
        self._port_combo.pack(side=tk.LEFT)

        self._conn_btn = tk.Button(ctrl, text="Connect", command=self._connect_toggle, bg='#4a1a1a', fg='#ff6b6b', relief=tk.FLAT, padx=8)
        self._conn_btn.pack(side=tk.LEFT, padx=4)

        self._ports_refresh()

    def _init_debug_panel(self):
        frame = tk.Frame(self._root, bg='#1e1e1e')
        frame.pack(fill=tk.X, padx=4, pady=(0, 4))

        sb = ttk.Scrollbar(frame)
        self._debug = tk.Text(frame, height=5, state=tk.DISABLED, bg='#111', fg='#888', font=('Courier', 9), yscrollcommand=sb.set, relief=tk.FLAT)
        sb.config(command=self._debug.yview)
        sb.pack(side=tk.RIGHT, fill=tk.Y)
        self._debug.pack(fill=tk.X)

        cmd_row = tk.Frame(self._root, bg='#1e1e1e')
        cmd_row.pack(fill=tk.X, padx=4, pady=(0, 4))

        self._cmd_var = tk.StringVar()
        cmd_entry = tk.Entry(cmd_row, textvariable=self._cmd_var, bg='#1e1e1e', fg='white', relief=tk.FLAT, insertbackground='white', font=('Courier', 9))
        cmd_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 4))
        cmd_entry.bind('<Return>', self._command_send)

        tk.Button(cmd_row, text="Send", command=self._command_send, bg='#3a3a3a', fg='white', relief=tk.FLAT, padx=8).pack(side=tk.LEFT)

    def _ports_refresh(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self._port_combo['values'] = ports

        if ports and not self._port_var.get():
            self._port_var.set(ports[0])

    def _connect_toggle(self):
        if self._reader.connected:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        self._ports_refresh()

        port = self._port_var.get()

        if not port:
            messagebox.showwarning("No Port", "Select a serial port first.")
            return

        try:
            self._reader.connect(port, self._cfg.DEFAULT_BAUD)
        except serial.SerialException as e:
            messagebox.showerror("Connection Failed", str(e))
            return

        self._stop.clear()

        threading.Thread(target=self._reader_thread, daemon=True).start()

        self._conn_btn.config(text="Disconnect", bg='#1a4a1a', fg='#00e676')

    def _disconnect(self):
        self._stop.set()

        if self._scope_running:
            self._reader.send(self._cfg.CMD_SCOPE_STOP)
            self._scope_running = False
            self._scope_btn.config(text="Start", bg='#1a4a1a', fg='#00e676')

        self._reader.disconnect()

        self._conn_btn.config(text="Connect", bg='#4a1a1a', fg='#ff6b6b')

    def _scope_toggle(self):
        if not self._reader.connected:
            self._debug_log('[WARN] Not connected')
            return

        if self._scope_running:
            self._reader.send(self._cfg.CMD_SCOPE_STOP)
            self._scope_running = False
            self._scope_btn.config(text="Start", bg='#1a4a1a', fg='#00e676')
        else:
            self._reader.send(self._cfg.CMD_SCOPE_START)
            self._scope_running = True
            self._scope_btn.config(text="Stop", bg='#4a1a1a', fg='#ff6b6b')

    def _pause_toggle(self):
        self._paused = not self._paused

        if not self._paused:
            self._buffer.reset()

        self._pause_btn.config(text="Resume" if self._paused else "Pause")

    def _cursor_toggle(self):
        active = not self._plot.cursor_active

        self._plot.cursor_set_active(active)
        self._cursor_btn.config(fg='#ffff00' if active else 'white')

    def _delta_toggle(self):
        active = not self._plot.delta_active

        self._plot.delta_set_active(active)
        self._delta_btn.config(fg='#ff9800' if active else 'white')

        if not active:
            self._delta_readout.config(text='')

    def _cursor_moved(self, y):
        self._cursor_readout.config(text=f"{y:.1f} mV" if y is not None else "")

    def _delta_moved(self, dt_ms):
        if dt_ms is None:
            self._delta_readout.config(text='')
            return

        dt_s   = dt_ms / 1000
        hz     = (1.0 / dt_s) if dt_s > 0 else 0
        hz_str = f"{hz / 1000:.3g} kHz" if hz >= 1000 else f"{hz:.3g} Hz"

        scale      = {'µs': (dt_ms * 1000, 'µs'), 'ms': (dt_ms, 'ms'), 's': (dt_s, 's')}
        val, unit  = scale[self._tb_unit]

        self._delta_readout.config(text=f"{val:.4g} {unit}\n{hz_str}")

    def _timebase_changed(self, val):
        pos = float(val) / 1000
        s   = self._cfg.TIMEBASE_MIN * (self._cfg.TIMEBASE_MAX / self._cfg.TIMEBASE_MIN) ** pos

        self._tb_seconds    = s
        self._buffer.target = max(1, int(s * self._cfg.ADC_SAMPLE_HZ))
        self._buffer.reset()

        if s < 1e-3:
            self._tb_unit = 'µs'
            self._tb_entry_var.set(f"{s * 1e6:.4g}")
        elif s < 1:
            self._tb_unit = 'ms'
            self._tb_entry_var.set(f"{s * 1e3:.4g}")
        else:
            self._tb_unit = 's'
            self._tb_entry_var.set(f"{s:.4g}")

        self._tb_unit_lbl.config(text=self._tb_unit)

    def _timebase_entry_apply(self, _event=None):
        try:
            raw = float(self._tb_entry_var.get())
        except ValueError:
            return

        s   = max(self._cfg.TIMEBASE_MIN, min(self._cfg.TIMEBASE_MAX, raw * {'µs': 1e-6, 'ms': 1e-3, 's': 1.0}[self._tb_unit]))
        pos = np.log(s / self._cfg.TIMEBASE_MIN) / np.log(self._cfg.TIMEBASE_MAX / self._cfg.TIMEBASE_MIN) * 1000

        self._tb_slider.set(pos)
        self._timebase_changed(pos)

    def _reader_thread(self):
        while not self._stop.is_set():
            try:
                frame = self._reader.read_frame()

                if frame:
                    self._queue.put(('frame', frame))
            except serial.SerialException as e:
                self._queue.put(('debug', f'[ERROR] {e}'))
                break

    def _queue_poll(self):
        try:
            while True:
                kind, data = self._queue.get_nowait()

                if kind == 'frame' and not self._paused:
                    if self._buffer.push(data.samples):
                        self._plot.redraw(self._buffer.view(), self._cfg.ADC_SAMPLE_HZ)
                        self._buffer.reset()
                elif kind == 'debug':
                    self._debug_log(data)
        except queue.Empty:
            pass

        self._root.after(20, self._queue_poll)

    def _debug_log(self, msg):
        self._debug.config(state=tk.NORMAL)
        self._debug.insert(tk.END, msg + '\n')
        self._debug.see(tk.END)
        self._debug.config(state=tk.DISABLED)

    def _command_send(self, _event=None):
        cmd = self._cmd_var.get().strip()

        if not cmd:
            return

        if not self._reader.connected:
            self._debug_log('[WARN] Not connected')
            return

        self._reader.send(cmd)
        self._debug_log(f'> {cmd}')
        self._cmd_var.set('')


def main():
    root = tk.Tk()
    OscilloscopeApp(root, Config())
    root.mainloop()


if __name__ == '__main__':
    main()
