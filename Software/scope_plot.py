import tkinter as tk
from tkinter import ttk

import numpy as np
import matplotlib
matplotlib.use('TkAgg')
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.transforms import blended_transform_factory


class ScopePlot:
    def __init__(self, parent, cfg, on_cursor_move=None, on_delta_move=None):
        self._cfg            = cfg
        self._on_cursor_move = on_cursor_move
        self._on_delta_move  = on_delta_move

        self._cursor_active = False
        self._cursor_drag   = False
        self._cursor_y      = 0.0

        self._delta_active = False
        self._delta_drag   = None
        self._delta_x      = [0.0, 0.0]

        self._v_min     = cfg.VOLTAGE_MIN
        self._v_max     = cfg.VOLTAGE_MAX
        self._autoscale = False

        frame = tk.Frame(parent, bg='#1e1e1e')
        frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        self._init_voltage_panel(frame)
        self._init_figure(frame)

    @property
    def cursor_active(self):
        return self._cursor_active

    @property
    def delta_active(self):
        return self._delta_active

    def cursor_set_active(self, active):
        self._cursor_active = active
        self._cursor_line.set_visible(active)
        self._cursor_label.set_visible(active)

        if not active and self._on_cursor_move:
            self._on_cursor_move(None)

        self._canvas.draw_idle()

    def delta_set_active(self, active):
        self._delta_active = active

        for line in self._delta_lines:
            line.set_visible(active)

        self._delta_label.set_visible(active)

        if active:
            xlim = self._ax.get_xlim()
            span = xlim[1] - xlim[0]
            self._delta_x = [xlim[0] + span * 0.25, xlim[0] + span * 0.75]
            self._delta_update()
        else:
            if self._on_delta_move:
                self._on_delta_move(None)
            self._canvas.draw_idle()

    def redraw(self, samples, sample_hz):
        t = np.arange(len(samples)) / sample_hz * 1000

        self._line.set_data(t, samples)
        self._ax.set_xlim(0, t[-1] if len(t) else 1)

        if self._autoscale:
            margin = max(50.0, (float(np.max(samples)) - float(np.min(samples))) * 0.1)
            self._ax.set_ylim(float(np.min(samples)) - margin, float(np.max(samples)) + margin)

        self._canvas.draw_idle()

    def _init_figure(self, parent):
        fig = Figure(figsize=(9, 4), facecolor='#1e1e1e')

        self._ax = fig.add_subplot(111, facecolor='#1e1e1e')
        self._ax.tick_params(colors='#aaa')
        self._ax.set_xlabel("Time (ms)", color='#aaa')
        self._ax.set_ylabel("Voltage (mV)", color='#aaa')
        self._ax.grid(True, color='#2a2a2a', lw=0.5)
        self._ax.set_ylim(self._cfg.VOLTAGE_MIN, self._cfg.VOLTAGE_MAX)

        for spine in self._ax.spines.values():
            spine.set_color('#444')

        self._line, = self._ax.plot([], [], color='#00e676', lw=0.8)

        self._canvas = FigureCanvasTkAgg(fig, parent)
        self._canvas.get_tk_widget().pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        trans_h = blended_transform_factory(self._ax.transAxes, self._ax.transData)
        self._cursor_line  = self._ax.axhline(y=0, color='#ffff00', alpha=0.35, lw=1, ls='--', visible=False)
        self._cursor_label = self._ax.text(0.01, 0, '', transform=trans_h, color='#ffff00', fontsize=8, va='bottom', visible=False)

        trans_v = blended_transform_factory(self._ax.transData, self._ax.transAxes)
        self._delta_lines = [
            self._ax.axvline(x=0, color='#ff9800', alpha=0.7, lw=1, ls='--', visible=False),
            self._ax.axvline(x=0, color='#ff9800', alpha=0.7, lw=1, ls='--', visible=False),
        ]
        self._delta_label = self._ax.text(0, 0.97, '', transform=trans_v, color='#ff9800', fontsize=8, ha='center', va='top', visible=False)

        self._canvas.mpl_connect('button_press_event',   self._mouse_press)
        self._canvas.mpl_connect('motion_notify_event',  self._mouse_motion)
        self._canvas.mpl_connect('button_release_event', self._mouse_release)

    def _init_voltage_panel(self, parent):
        lbl = dict(bg='#2a2a2a', fg='#aaa', font=('', 8))
        ent = dict(width=5, bg='#1e1e1e', fg='white', relief=tk.FLAT, insertbackground='white', font=('Courier', 9), justify='center')

        panel = tk.Frame(parent, bg='#2a2a2a', width=80)
        panel.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 2))
        panel.pack_propagate(False)

        tk.Label(panel, text="MAX", **lbl).pack(pady=(6, 0))

        self._vmax_var = tk.StringVar()
        e = tk.Entry(panel, textvariable=self._vmax_var, **ent)
        e.bind('<Return>', self._vmax_entry_apply)
        e.pack(pady=2)

        sliders = tk.Frame(panel, bg='#2a2a2a')
        sliders.pack(fill=tk.BOTH, expand=True)

        self._vmax_slider = ttk.Scale(sliders, from_=self._cfg.VOLTAGE_SLIDER_MAX, to=0, orient=tk.VERTICAL, command=self._vmax_changed)
        self._vmax_slider.pack(side=tk.LEFT, fill=tk.Y, expand=True, padx=2, pady=4)

        self._vmin_slider = ttk.Scale(sliders, from_=self._cfg.VOLTAGE_SLIDER_MAX, to=0, orient=tk.VERTICAL, command=self._vmin_changed)
        self._vmin_slider.pack(side=tk.LEFT, fill=tk.Y, expand=True, padx=2, pady=4)

        self._vmin_var = tk.StringVar()
        e = tk.Entry(panel, textvariable=self._vmin_var, **ent)
        e.bind('<Return>', self._vmin_entry_apply)
        e.pack(pady=2)

        tk.Label(panel, text="MIN", **lbl).pack()

        self._autoscale_btn = tk.Button(panel, text="Auto", command=self._autoscale_toggle, bg='#3a3a3a', fg='#666', relief=tk.FLAT, font=('', 8))
        self._autoscale_btn.pack(fill=tk.X, padx=4, pady=(4, 6))

        self._vmax_slider.set(self._cfg.VOLTAGE_MAX)
        self._vmin_slider.set(self._cfg.VOLTAGE_MIN)
        self._vmax_var.set(f"{self._cfg.VOLTAGE_MAX:.0f}")
        self._vmin_var.set(f"{self._cfg.VOLTAGE_MIN:.0f}")

    def _vmax_changed(self, val):
        self._v_max = float(val)
        self._vmax_var.set(f"{self._v_max:.0f}")

        if not self._autoscale and hasattr(self, '_ax'):
            self._ax.set_ylim(self._v_min, max(self._v_max, self._v_min + 1))
            self._canvas.draw_idle()

    def _vmin_changed(self, val):
        self._v_min = float(val)
        self._vmin_var.set(f"{self._v_min:.0f}")

        if not self._autoscale and hasattr(self, '_ax'):
            self._ax.set_ylim(self._v_min, max(self._v_max, self._v_min + 1))
            self._canvas.draw_idle()

    def _vmax_entry_apply(self, *_):
        try:
            v = float(self._vmax_var.get())
        except ValueError:
            return

        self._vmax_slider.set(max(0.0, min(self._cfg.VOLTAGE_SLIDER_MAX, v)))
        self._vmax_changed(v)

    def _vmin_entry_apply(self, *_):
        try:
            v = float(self._vmin_var.get())
        except ValueError:
            return

        self._vmin_slider.set(max(0.0, min(self._cfg.VOLTAGE_SLIDER_MAX, v)))
        self._vmin_changed(v)

    def _autoscale_toggle(self):
        self._autoscale = not self._autoscale

        self._autoscale_btn.config(text="Auto ON" if self._autoscale else "Auto", fg='#00e676' if self._autoscale else '#666')

        if not self._autoscale:
            self._ax.set_ylim(self._v_min, self._v_max)
            self._canvas.draw_idle()

    def _mouse_press(self, event):
        if event.inaxes is not self._ax or event.xdata is None or event.ydata is None:
            return

        if self._delta_active and self._cursor_active:
            def x_px(xd): return abs(event.x - self._ax.transData.transform((xd, 0))[0])
            def y_px(yd): return abs(event.y - self._ax.transData.transform((0, yd))[1])

            dist_d0     = x_px(self._delta_x[0])
            dist_d1     = x_px(self._delta_x[1])
            dist_cursor = y_px(self._cursor_y)

            if dist_cursor < min(dist_d0, dist_d1):
                self._cursor_drag = True
                self._cursor_y    = event.ydata
                self._cursor_update()
            else:
                self._delta_drag = 0 if dist_d0 <= dist_d1 else 1
                self._delta_x[self._delta_drag] = event.xdata
                self._delta_update()

        elif self._delta_active:
            dist_d0 = abs(event.xdata - self._delta_x[0])
            dist_d1 = abs(event.xdata - self._delta_x[1])

            self._delta_drag = 0 if dist_d0 <= dist_d1 else 1
            self._delta_x[self._delta_drag] = event.xdata
            self._delta_update()

        elif self._cursor_active:
            self._cursor_drag = True
            self._cursor_y    = event.ydata
            self._cursor_update()

    def _mouse_motion(self, event):
        if event.inaxes is not self._ax:
            return

        if self._delta_drag is not None and event.xdata is not None:
            self._delta_x[self._delta_drag] = event.xdata
            self._delta_update()

        elif self._cursor_drag and event.ydata is not None:
            self._cursor_y = event.ydata
            self._cursor_update()

    def _mouse_release(self, *_):
        self._cursor_drag = False
        self._delta_drag  = None

    def _cursor_update(self):
        self._cursor_line.set_ydata([self._cursor_y, self._cursor_y])
        self._cursor_label.set_y(self._cursor_y)
        self._cursor_label.set_text(f"{self._cursor_y:.1f} mV")

        if self._on_cursor_move:
            self._on_cursor_move(self._cursor_y)

        self._canvas.draw_idle()

    def _delta_update(self):
        for line, x in zip(self._delta_lines, self._delta_x):
            line.set_xdata([x, x])

        mid    = (self._delta_x[0] + self._delta_x[1]) / 2
        dt_ms  = abs(self._delta_x[1] - self._delta_x[0])
        dt_s   = dt_ms / 1000
        hz     = (1.0 / dt_s) if dt_s > 0 else 0
        hz_str = f"{hz / 1000:.3g} kHz" if hz >= 1000 else f"{hz:.3g} Hz"
        t_str  = f"{dt_ms * 1000:.2f} µs" if dt_ms < 1 else f"{dt_ms:.4g} ms"

        self._delta_label.set_x(mid)
        self._delta_label.set_text(f"Δt={t_str}  {hz_str}")

        if self._on_delta_move:
            self._on_delta_move(dt_ms)

        self._canvas.draw_idle()
