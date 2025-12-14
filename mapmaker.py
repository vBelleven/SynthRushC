import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog

# --- PALETA DE COLORES SYNTHWAVE ---
THEME = {
    'BG': '#050510',        # Fondo casi negro (Dark Navy)
    'GRID': '#1a1a2e',      # Líneas de la grilla (Azul oscuro)
    'PANEL': '#101018',     # Panel lateral
    'TEXT': '#e0e0e0',      # Texto general
    'ACCENT': '#ff00ff'     # Acentos
}

# Colores de las celdas (Lo que se pinta)
CELL_COLORS = {
    '@': '#ff0055',  # Muro (Neon Red/Pink) - Bloque
    '!': '#00f0ff',  # Torre (Neon Cyan)
    '*': '#ff9900',  # Enemigo (Neon Orange)
    ' ': '#050510',  # Vacío (Igual al fondo)
    'P': '#39ff14'   # Jugadores (Neon Green)
}

# --- CONFIGURACIÓN DEL MAPA ---
MAP_WIDTH = 60
MAP_HEIGHT = 60
CELL_SIZE = 20  # Píxeles por bloque en el editor

class SynthMapEditor:
    def __init__(self, root):
        self.root = root
        self.root.title(f"SYNTHRUSH LEVEL EDITOR - {MAP_WIDTH}x{MAP_HEIGHT}")
        self.root.configure(bg=THEME['BG'])

        # Datos del mapa
        self.rows = MAP_HEIGHT
        self.cols = MAP_WIDTH
        self.grid_data = [[' ' for _ in range(self.cols)] for _ in range(self.rows)]
        self.rects = [[None for _ in range(self.cols)] for _ in range(self.rows)]
        
        # Estado del Pincel
        self.current_tool = '@' 
        self.current_player_num = '1' # Por defecto Jugador 1

        # --- INTERFAZ ---
        self.setup_ui()
        self.setup_grid()
        self.bind_controls()

    def setup_ui(self):
        # --- BARRA LATERAL (HERRAMIENTAS) ---
        toolbar = tk.Frame(self.root, bg=THEME['PANEL'], width=200, relief=tk.RAISED, bd=0)
        toolbar.pack(side=tk.LEFT, fill=tk.Y)

        # Título
        tk.Label(toolbar, text="SYNTHRUSH", fg="#00f0ff", bg=THEME['PANEL'], 
                 font=("Impact", 18)).pack(pady=(15, 5))
        tk.Label(toolbar, text="EDITOR v2.0", fg="#ff0055", bg=THEME['PANEL'], 
                 font=("Arial", 10, "bold")).pack(pady=(0, 20))

        # Sección: Archivo
        self.create_header(toolbar, "ARCHIVO")
        self.create_btn(toolbar, "CARGAR .TXT", self.load_map, "#333", "white")
        self.create_btn(toolbar, "GUARDAR .TXT", self.save_map, "#333", "white")
        self.create_btn(toolbar, "LIMPIAR TODO", self.clear_map, "#552222", "white")
        
        # Espaciador
        tk.Frame(toolbar, height=2, bg=THEME['GRID']).pack(fill=tk.X, pady=15)

        # Sección: Herramientas
        self.create_header(toolbar, "BROCHAS")
        
        self.btn_wall = self.create_tool_btn(toolbar, "[W] MURO (@)", '@', CELL_COLORS['@'])
        self.btn_tower = self.create_tool_btn(toolbar, "[T] TORRE (!)", '!', CELL_COLORS['!'])
        self.btn_enemy = self.create_tool_btn(toolbar, "[E] ENEMIGO (*)", '*', CELL_COLORS['*'])
        
        # Botón Especial de Jugador
        self.btn_player = tk.Button(toolbar, text="[P] SPAWN JUGADOR", 
                                    bg=CELL_COLORS['P'], fg="black", font=("Arial", 10, "bold"),
                                    activebackground="white", relief=tk.FLAT,
                                    command=self.ask_player_number)
        self.btn_player.pack(fill=tk.X, padx=10, pady=5)

        self.btn_erase = self.create_tool_btn(toolbar, "[ESPACIO] BORRAR", ' ', "#333")

        # Indicador de estado
        tk.Frame(toolbar, height=2, bg=THEME['GRID']).pack(fill=tk.X, pady=15)
        self.lbl_status = tk.Label(toolbar, text="HERRAMIENTA:\nMURO", fg="white", bg=THEME['PANEL'], 
                                   font=("Consolas", 12))
        self.lbl_status.pack(side=tk.BOTTOM, pady=20)

        # --- ÁREA DE DIBUJO (CANVAS) ---
        self.canvas_frame = tk.Frame(self.root, bg=THEME['BG'])
        self.canvas_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        
        # Scrollbars (Estilo oscuro si el SO lo permite, si no, standard)
        h_scroll = tk.Scrollbar(self.canvas_frame, orient=tk.HORIZONTAL, bg=THEME['PANEL'])
        v_scroll = tk.Scrollbar(self.canvas_frame, orient=tk.VERTICAL, bg=THEME['PANEL'])
        
        self.canvas = tk.Canvas(self.canvas_frame, bg=THEME['BG'], highlightthickness=0,
                                scrollregion=(0, 0, self.cols*CELL_SIZE, self.rows*CELL_SIZE),
                                xscrollcommand=h_scroll.set, yscrollcommand=v_scroll.set)
        
        h_scroll.config(command=self.canvas.xview)
        v_scroll.config(command=self.canvas.yview)
        
        h_scroll.pack(side=tk.BOTTOM, fill=tk.X)
        v_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    # --- HELPERS DE UI ---
    def create_header(self, parent, text):
        tk.Label(parent, text=text, fg="#666", bg=THEME['PANEL'], 
                 font=("Arial", 8, "bold")).pack(anchor=tk.W, padx=10)

    def create_btn(self, parent, text, cmd, bg, fg):
        btn = tk.Button(parent, text=text, command=cmd, bg=bg, fg=fg, 
                        relief=tk.FLAT, activebackground="#555")
        btn.pack(fill=tk.X, padx=10, pady=2)
        return btn

    def create_tool_btn(self, parent, text, char, color):
        fg_col = "black" if char in ['!', 'P', '*', '@'] else "white"
        btn = tk.Button(parent, text=text, bg=color, fg=fg_col, font=("Arial", 9, "bold"),
                        relief=tk.FLAT, activebackground="white",
                        command=lambda: self.set_tool(char))
        btn.pack(fill=tk.X, padx=10, pady=2)
        return btn

    def setup_grid(self):
        # Dibuja la cuadrícula vacía
        for r in range(self.rows):
            for c in range(self.cols):
                x1 = c * CELL_SIZE
                y1 = r * CELL_SIZE
                x2 = x1 + CELL_SIZE
                y2 = y1 + CELL_SIZE
                # Borde sutil color grid
                rect = self.canvas.create_rectangle(x1, y1, x2, y2, 
                                                    fill=THEME['BG'], outline=THEME['GRID'])
                self.rects[r][c] = rect

    def bind_controls(self):
        self.canvas.bind("<Button-1>", self.paint_cell) # Click
        self.canvas.bind("<B1-Motion>", self.paint_cell) # Arrastrar
        self.canvas.bind("<Button-3>", self.erase_cell) # Click Derecho

        # Atajos
        self.root.bind("w", lambda e: self.set_tool('@'))
        self.root.bind("t", lambda e: self.set_tool('!'))
        self.root.bind("e", lambda e: self.set_tool('*'))
        self.root.bind("p", lambda e: self.ask_player_number()) # P para Player
        self.root.bind("<space>", lambda e: self.set_tool(' '))

    # --- LÓGICA DE HERRAMIENTAS ---

    def set_tool(self, char):
        self.current_tool = char
        
        # Feedback visual en etiqueta
        names = {'@': 'MURO', '!': 'TORRE', '*': 'ENEMIGO', ' ': 'BORRADOR'}
        name = names.get(char, f"JUGADOR {self.current_tool}")
        
        color = CELL_COLORS.get(char, CELL_COLORS['P'])
        self.lbl_status.config(text=f"HERRAMIENTA:\n{name}", fg=color)

    def ask_player_number(self):
        # Pregunta al usuario qué número quiere usar
        num = simpledialog.askinteger("Spawn de Jugador", 
                                      "¿Qué número de jugador asignar? (1-8)",
                                      minvalue=1, maxvalue=8, parent=self.root)
        if num is not None:
            self.current_player_num = str(num)
            self.set_tool(str(num)) # La herramienta ahora es "1", "2", etc.

    def paint_cell(self, event):
        # Mapear click a coordenadas de grid
        col = int(self.canvas.canvasx(event.x) // CELL_SIZE)
        row = int(self.canvas.canvasy(event.y) // CELL_SIZE)

        if 0 <= row < self.rows and 0 <= col < self.cols:
            char = self.current_tool
            
            # Guardar en memoria
            self.grid_data[row][col] = char
            
            # Determinar color
            color = CELL_COLORS.get(char, CELL_COLORS[' '])
            if char in '12345678': color = CELL_COLORS['P'] 
            
            # Pintar celda
            self.canvas.itemconfig(self.rects[row][col], fill=color, outline=THEME['GRID'])
            
            # Manejar texto de números (Si es jugador)
            tags = f"text_{row}_{col}"
            self.canvas.delete(tags) # Borrar texto anterior si había
            
            if char in '12345678':
                x = col * CELL_SIZE + CELL_SIZE/2
                y = row * CELL_SIZE + CELL_SIZE/2
                self.canvas.create_text(x, y, text=char, fill="black", 
                                      font=("Impact", 10), tags=tags)
            elif char == '!':
                # Dibujar una 'T' pequeña para la torre
                x = col * CELL_SIZE + CELL_SIZE/2
                y = row * CELL_SIZE + CELL_SIZE/2
                self.canvas.create_text(x, y, text="T", fill="black", 
                                      font=("Arial", 10, "bold"), tags=tags)

    def erase_cell(self, event):
        prev = self.current_tool
        self.current_tool = ' '
        self.paint_cell(event)
        self.current_tool = prev

    # --- IO (GUARDAR/CARGAR) ---

    def save_map(self):
        filename = filedialog.asksaveasfilename(defaultextension=".txt", 
                                              filetypes=[("Text Files", "*.txt")],
                                              initialfile="level1.txt")
        if not filename: return
        
        try:
            with open(filename, 'w') as f:
                for row in self.grid_data:
                    line = "".join(row)
                    f.write(line + "\n")
            messagebox.showinfo("GUARDADO", "Nivel exportado exitosamente.")
        except Exception as e:
            messagebox.showerror("ERROR", f"No se pudo guardar:\n{e}")

    def load_map(self):
        filename = filedialog.askopenfilename(filetypes=[("Text Files", "*.txt")])
        if not filename: return

        try:
            with open(filename, 'r') as f:
                lines = [line.rstrip('\n') for line in f.readlines()]
            
            self.clear_map()
            
            # Cargar datos visuales
            for r in range(min(len(lines), self.rows)):
                for c in range(min(len(lines[r]), self.cols)):
                    char = lines[r][c]
                    # Simular pintado programático
                    self.current_tool = char
                    
                    # Hack para pintar sin evento de mouse
                    self.grid_data[r][c] = char
                    
                    color = CELL_COLORS.get(char, CELL_COLORS[' '])
                    if char in '12345678': color = CELL_COLORS['P']
                    
                    self.canvas.itemconfig(self.rects[r][c], fill=color)
                    
                    # Repintar números
                    if char in '12345678':
                        tags = f"text_{r}_{c}"
                        x = c * CELL_SIZE + CELL_SIZE/2
                        y = r * CELL_SIZE + CELL_SIZE/2
                        self.canvas.create_text(x, y, text=char, fill="black", 
                                              font=("Impact", 10), tags=tags)
                    elif char == '!':
                        tags = f"text_{r}_{c}"
                        x = c * CELL_SIZE + CELL_SIZE/2
                        y = r * CELL_SIZE + CELL_SIZE/2
                        self.canvas.create_text(x, y, text="T", fill="black", 
                                              font=("Arial", 10, "bold"), tags=tags)

            self.set_tool('@') # Reset a Muro
            
        except Exception as e:
            messagebox.showerror("ERROR", f"Error al cargar:\n{e}")

    def clear_map(self):
        self.grid_data = [[' ' for _ in range(self.cols)] for _ in range(self.rows)]
        for r in range(self.rows):
            for c in range(self.cols):
                self.canvas.itemconfig(self.rects[r][c], fill=THEME['BG'])
                self.canvas.delete(f"text_{r}_{c}")

if __name__ == "__main__":
    root = tk.Tk()
    # Tamaño ajustado para 60 columnas * 20px + barra lateral
    window_w = (MAP_WIDTH * CELL_SIZE) + 240
    window_h = (MAP_HEIGHT * CELL_SIZE) + 40
    # Limitar tamaño máximo si la pantalla es pequeña
    screen_w = root.winfo_screenwidth()
    screen_h = root.winfo_screenheight()
    
    final_w = min(window_w, screen_w - 100)
    final_h = min(window_h, screen_h - 100)
    
    root.geometry(f"{final_w}x{final_h}")
    
    app = SynthMapEditor(root)
    root.mainloop()