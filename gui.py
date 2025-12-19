import tkinter as tk
from tkinter import ttk
import subprocess
import threading
import sys

# Configuration
EXE_PATH = "./build/dsm_headless"
CONFIG_FILE = "cluster.txt"

class DsmGui:
    def __init__(self, root, node_id):
        self.root = root
        self.node_id = node_id
        self.root.title(f"DSM Node {node_id} - Dashboard")
        self.root.geometry("600x700")

        style = ttk.Style()
        style.configure("Treeview",
                        background="#2e2e2e",
                        fieldbackground="#2e2e2e",
                        foreground="white")

        style.map("Treeview",
                  background=[('selected', '#007aff')],
                  foreground=[('selected', 'white')])

        style.configure("Green.TLabel", foreground="#00ff00", font=("Arial", 12, "bold"))
        style.configure("Red.TLabel", foreground="#ff4444", font=("Arial", 12, "bold"))
        style.configure("Blue.TLabel", foreground="#4488ff", font=("Arial", 12, "bold"))
        style.configure("Std.TLabel", foreground="#eeeeee", font=("Courier", 12, "bold"))

        self.status_frame = ttk.LabelFrame(root, text="System Status")
        self.status_frame.pack(fill="x", padx=10, pady=5)
        self.lbl_status = ttk.Label(self.status_frame, text="STARTING...", style="Blue.TLabel")
        self.lbl_status.pack(pady=5)

        self.result_frame = ttk.LabelFrame(root, text="Server Response")
        self.result_frame.pack(fill="x", padx=10, pady=5)
        self.lbl_result = ttk.Label(self.result_frame, text="Ready.", style="Std.TLabel")
        self.lbl_result.pack(pady=5)

        self.cache_frame = ttk.LabelFrame(root, text="Node Storage (Home & Cache)")
        self.cache_frame.pack(fill="both", expand=True, padx=10, pady=5)

        self.tree = ttk.Treeview(self.cache_frame, columns=("Key", "Value", "Role"), show="headings", height=8)
        self.tree.heading("Key", text="Object Key")
        self.tree.heading("Value", text="Data Value")
        self.tree.heading("Role", text="Ownership Role")

        self.tree.column("Key", width=120)
        self.tree.column("Value", width=200)
        self.tree.column("Role", width=120)

        self.tree.tag_configure('home_row', background='#1e4d2b', foreground='white')
        self.tree.tag_configure('backup_row', background='#5c3a00', foreground='white')

        self.tree.pack(fill="both", expand=True, padx=5, pady=5)

        self.ctrl_frame = ttk.LabelFrame(root, text="Operations")
        self.ctrl_frame.pack(fill="x", padx=10, pady=5)

        tk.Label(self.ctrl_frame, text="Key:").grid(row=0, column=0, padx=5, pady=5)
        self.entry_key = tk.Entry(self.ctrl_frame, width=15)
        self.entry_key.grid(row=0, column=1, padx=5, pady=5)

        tk.Label(self.ctrl_frame, text="Value:").grid(row=0, column=2, padx=5, pady=5)
        self.entry_val = tk.Entry(self.ctrl_frame, width=15)
        self.entry_val.grid(row=0, column=3, padx=5, pady=5)

        self.btn_get = ttk.Button(self.ctrl_frame, text="GET", command=self.do_get)
        self.btn_get.grid(row=1, column=0, sticky="ew", padx=5, pady=5)

        self.btn_slow_get = ttk.Button(self.ctrl_frame, text="SLOW READ (5s)", command=self.do_slow_get)
        self.btn_slow_get.grid(row=1, column=1, sticky="ew", padx=5, pady=5)

        self.btn_put = ttk.Button(self.ctrl_frame, text="PUT", command=self.do_put)
        self.btn_put.grid(row=1, column=2, sticky="ew", padx=5, pady=5)

        self.btn_slow_put = ttk.Button(self.ctrl_frame, text="SLOW WRITE (5s)", command=self.do_slow_put)
        self.btn_slow_put.grid(row=1, column=3, sticky="ew", padx=5, pady=5)

        self.start_cpp_process()

    def start_cpp_process(self):
        try:
            self.proc = subprocess.Popen(
                [EXE_PATH, str(self.node_id), CONFIG_FILE],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1
            )
            threading.Thread(target=self.read_loop, daemon=True).start()
        except FileNotFoundError:
            self.lbl_status.config(text=f"ERROR: Missing {EXE_PATH}", style="Red.TLabel")

    def read_loop(self):
        while True:
            try:
                line = self.proc.stdout.readline()
                if not line: break
                line = line.strip()
                if "|" in line:
                    parts = line.split("|", 1)
                    self.root.after(0, self.handle_update, parts[0], parts[1])
                else:
                    print(f"[CPP] {line}")
            except Exception as e:
                print(f"Read Loop Error: {e}")
                break

    def handle_update(self, msg_type, payload):
        if msg_type == "STATUS":
            self.lbl_status.config(text=payload)
            if "LOCKING" in payload or "HELD" in payload:
                self.lbl_status.config(style="Red.TLabel")
            elif "IDLE" in payload:
                self.lbl_status.config(style="Green.TLabel")
            else:
                self.lbl_status.config(style="Blue.TLabel")

        elif msg_type == "RESULT":
            style = "Std.TLabel"
            if "NOT FOUND" in payload:
                style = "Red.TLabel"
            self.lbl_result.config(text=payload, style=style)

        elif msg_type == "CACHE":
            parts = payload.split("|")
            if len(parts) >= 3:
                self.update_tree(parts[0], parts[1], parts[2])

        elif msg_type == "ERROR":
            self.lbl_status.config(text=f"ERROR: {payload}", style="Red.TLabel")

    def update_tree(self, key, val, role):
        tag = ""
        if "HOME" in role: tag = "home_row"
        elif "BACKUP" in role: tag = "backup_row"

        for item in self.tree.get_children():
            item_key = self.tree.item(item, "values")[0]
            if item_key == key:
                self.tree.item(item, values=(key, val, role), tags=(tag,))
                return
        self.tree.insert("", "end", values=(key, val, role), tags=(tag,))

    def send_cmd(self, cmd):
        if self.proc.poll() is None:
            self.proc.stdin.write(cmd + "\n")
            self.proc.stdin.flush()

    def do_get(self):
        if k := self.entry_key.get(): self.send_cmd(f"get {k}")

    def do_slow_get(self):
        if k := self.entry_key.get(): self.send_cmd(f"slowget {k}")

    def do_put(self):
        if (k := self.entry_key.get()) and (v := self.entry_val.get()):
            self.send_cmd(f"put {k} {v}")

    def do_slow_put(self):
        if (k := self.entry_key.get()) and (v := self.entry_val.get()):
            self.send_cmd(f"slowput {k} {v}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 gui.py <node_id>")
        sys.exit(1)
    root = tk.Tk()
    DsmGui(root, sys.argv[1])
    root.mainloop()