import tkinter as tk
from tkinter import ttk, messagebox
import threading
import traceback
from datetime import datetime
import automate_generic
from imagesearchdll import get_windows_display_essentials

# Manual imports - these will be included in the EXE
import automate_alarms
import automate_certificate_settings

# Import the ScriptExitException from your actual automation scripts
try:
    from imagesearchdll import ScriptExitException
except ImportError:
    # Fallback definition if imagesearchdll is not available
    class ScriptExitException(Exception):
        """Custom exception to gracefully exit script execution"""
        def __init__(self, message="Script execution terminated", return_value=False):
            self.message = message
            self.return_value = return_value
            super().__init__(self.message)

class ScriptInfo:
    def __init__(self, module):
        self.module = module
        self.name = getattr(module, 'ScriptName', module.__name__)
        self.description = getattr(module, 'ScriptDescription', 'No description available')
        self.button = None  # Will store reference to the button widget
        self.execution_status = None  # Track last execution status
    
    def execute_full_automation(self):
        """Execute the complete automation cycle with ScriptExitException handling"""
        results = {}
        early_exit = False
        exit_message = None
        
        try:
            # Execute init_automation
            if hasattr(self.module, 'init_automation'):
                print(f"🔧 Initializing {self.name}...")
                try:
                    init_result = self.module.init_automation()
                    results['init'] = init_result
                    print(f"✅ Initialization completed")
                except ScriptExitException as e:
                    print(f"🛑 Initialization terminated: {e.message}")
                    results['init'] = e.return_value
                    early_exit = True
                    exit_message = f"Init phase: {e.message}"
                    # Don't proceed to run phase if init failed
                    if e.return_value is False:
                        raise e
            
            # Execute run_automation (only if init didn't fail)
            if not early_exit and hasattr(self.module, 'run_automation'):
                print(f"▶️ Running {self.name}...")
                try:
                    run_result = self.module.run_automation()
                    results['run'] = run_result
                    print(f"✅ Execution completed")
                except ScriptExitException as e:
                    print(f"🛑 Execution terminated: {e.message}")
                    results['run'] = e.return_value
                    early_exit = True
                    exit_message = f"Run phase: {e.message}"
        
        except ScriptExitException as e:
            # This catches exits from init phase that should stop everything
            early_exit = True
            exit_message = f"Critical exit: {e.message}"
            results['exit_reason'] = e.message
            results['exit_return_value'] = e.return_value
        
        # Always attempt cleanup, regardless of early exit
        if hasattr(self.module, 'cleanup_automation'):
            print(f"🧹 Cleaning up {self.name}...")
            try:
                cleanup_result = self.module.cleanup_automation()
                results['cleanup'] = cleanup_result
                print(f"✅ Cleanup completed")
            except ScriptExitException as e:
                print(f"🛑 Cleanup terminated: {e.message}")
                results['cleanup'] = e.return_value
                if not early_exit:  # Only set exit message if not already set
                    early_exit = True
                    exit_message = f"Cleanup phase: {e.message}"
            except Exception as e:
                print(f"❌ Cleanup failed with error: {str(e)}")
                results['cleanup'] = False
        
        # Add exit information to results
        if early_exit:
            results['early_exit'] = True
            results['exit_message'] = exit_message
        
        return results

class ScriptManager:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("🤖 Automation Script Manager")
        self.root.geometry("800x600")
        self.root.configure(bg='#f0f0f0')
        
        # Manually define scripts to include in EXE
        self.scripts_to_list = [
            ScriptInfo(automate_alarms),
            ScriptInfo(automate_certificate_settings)
        ]
        
        # Create custom styles for colored buttons
        self.setup_button_styles()
        
        self.setup_ui()
        self.log_output("Script Manager initialized successfully!")
        self.log_output(f"Loaded {len(self.scripts_to_list)} automation scripts.")
        
        # make scripts output to our window
        automate_generic.set_log_fnc(self.log_output)
        
        # expose visually current system resolution
        resolution_info = get_windows_display_essentials()
        self.log_output(f"{resolution_info['logical_resolution']['width']}x{resolution_info['logical_resolution']['height']} zoom: {resolution_info['zoom_percent']}, scale: {resolution_info['scale_factor']}")
    
    def setup_button_styles(self):
        """Create custom styles for button states"""
        self.style = ttk.Style()
        
        # Success style (green)
        self.style.configure(
            "Success.TButton",
            background="#d4edda",  # Light green background
            foreground="#155724",  # Dark green text
            bordercolor="#c3e6cb",
            focuscolor="none"
        )
        
        # Map for hover effects on success buttons
        self.style.map(
            "Success.TButton",
            background=[
                ("active", "#c3e6cb"),  # Slightly darker green on hover
                ("pressed", "#b8dabd")   # Even darker when pressed
            ]
        )
        
        # Failure style (red)
        self.style.configure(
            "Failure.TButton",
            background="#f8777a",  # Light red background
            foreground="#721c24",  # Dark red text
            bordercolor="#f1b0b7",
            focuscolor="none"
        )
        
        # Map for hover effects on failure buttons
        self.style.map(
            "Failure.TButton",
            background=[
                ("active", "#f1b0b7"),  # Slightly darker red on hover
                ("pressed", "#ea868f")   # Even darker when pressed
            ]
        )
        
        # Running style (yellow/orange)
        self.style.configure(
            "Running.TButton",
            background="#fff3cd",  # Light yellow background
            foreground="#856404",  # Dark yellow/brown text
            bordercolor="#ffeaa7",
            focuscolor="none"
        )
        
        # Map for hover effects on running buttons
        self.style.map(
            "Running.TButton",
            background=[
                ("active", "#ffeaa7"),  # Slightly darker yellow on hover
                ("pressed", "#fdcb6e")   # Even darker when pressed
            ]
        )
        
        # Reset style (default)
        self.style.configure(
            "Reset.TButton",
            background="#e9ecef",  # Light gray background
            foreground="#495057",  # Dark gray text
            bordercolor="#ced4da",
            focuscolor="none"
        )
    
    def update_button_status(self, script_info, status):
        """Update button appearance based on execution status"""
        if script_info.button is None:
            return
        
        script_info.execution_status = status
        
        if status == "success":
            script_info.button.configure(style="Success.TButton")
            # Update button text to show success
            current_text = script_info.button.cget("text")
            if not current_text.startswith("✅"):
                base_text = current_text.replace("🚀", "").replace("❌", "").replace("⚠️", "").strip()
                script_info.button.configure(text=f"✅ {base_text}")
        
        elif status == "failure":
            script_info.button.configure(style="Failure.TButton")
            # Update button text to show failure
            current_text = script_info.button.cget("text")
            if not current_text.startswith("❌"):
                base_text = current_text.replace("🚀", "").replace("✅", "").replace("⚠️", "").strip()
                script_info.button.configure(text=f"❌ {base_text}")
        
        elif status == "warning":
            script_info.button.configure(style="Failure.TButton")  # Use red style for warnings too
            # Update button text to show warning
            current_text = script_info.button.cget("text")
            if not current_text.startswith("⚠️"):
                base_text = current_text.replace("🚀", "").replace("✅", "").replace("❌", "").strip()
                script_info.button.configure(text=f"⚠️ {base_text}")
        
        elif status == "running":
            script_info.button.configure(style="Running.TButton")
            # Update button text to show running
            current_text = script_info.button.cget("text")
            base_text = current_text.replace("🚀", "").replace("✅", "").replace("❌", "").replace("⚠️", "").strip()
            script_info.button.configure(text=f"🔄 {base_text}")
        
        elif status == "reset":
            script_info.button.configure(style="Reset.TButton")
            # Reset button text
            script_info.button.configure(text=f"🚀 {script_info.name}")
        
        # Force UI update
        self.root.update_idletasks()
    
    def reset_all_button_statuses(self):
        """Reset all buttons to default state"""
        for script_info in self.scripts_to_list:
            self.update_button_status(script_info, "reset")
    
    def setup_ui(self):
        # Configure root grid
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        
        # Main container
        main_container = ttk.Frame(self.root, padding="20")
        main_container.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        main_container.columnconfigure(0, weight=1)
        main_container.rowconfigure(2, weight=1)
        
        # Title section
        self.create_title_section(main_container)
        
        # Control buttons
        self.create_control_section(main_container)
        
        # Scripts section
        self.create_scripts_section(main_container)
        
        # Output section
        self.create_output_section(main_container)
    
    def create_title_section(self, parent):
        title_frame = ttk.Frame(parent)
        title_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 25))
        title_frame.columnconfigure(0, weight=1)
        
        title_label = ttk.Label(title_frame, text="🤖 Automation Script Manager", 
                               font=("Segoe UI", 20, "bold"))
        title_label.grid(row=0, column=0)
        
        subtitle_label = ttk.Label(title_frame, text="Execute automation scripts with visual status feedback", 
                                  font=("Segoe UI", 11), foreground="gray")
        subtitle_label.grid(row=1, column=0, pady=(8, 0))
    
    def create_control_section(self, parent):
        control_frame = ttk.LabelFrame(parent, text="🎛️ Controls", padding="15")
        control_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=(0, 20))
        
        # Create a frame for buttons to center them
        button_frame = ttk.Frame(control_frame)
        button_frame.pack(expand=True)
        
        ttk.Button(button_frame, text="🚀 Run All Scripts", 
                  command=self.run_all_scripts, width=22).pack(side=tk.LEFT, padx=10)
        ttk.Button(button_frame, text="📋 Show Script Details", 
                  command=self.show_script_info, width=22).pack(side=tk.LEFT, padx=10)
        ttk.Button(button_frame, text="🔄 Reset Status", 
                  command=self.reset_all_button_statuses, width=22).pack(side=tk.LEFT, padx=10)
        ttk.Button(button_frame, text="🗑️ Clear Output", 
                  command=self.clear_output, width=22).pack(side=tk.LEFT, padx=10)
    
    def create_scripts_section(self, parent):
        scripts_frame = ttk.LabelFrame(parent, text="📄 Available Scripts", padding="15")
        scripts_frame.grid(row=2, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(0, 20))
        scripts_frame.columnconfigure(0, weight=1)
        scripts_frame.rowconfigure(0, weight=1)
        
        # Create canvas and scrollbar for scrolling
        canvas = tk.Canvas(scripts_frame, bg='white', highlightthickness=0)
        scrollbar = ttk.Scrollbar(scripts_frame, orient="vertical", command=canvas.yview)
        self.scrollable_frame = ttk.Frame(canvas)
        
        self.scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=self.scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        canvas.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        
        # Bind mousewheel to canvas
        def _on_mousewheel(event):
            canvas.yview_scroll(int(-1*(event.delta/120)), "units")
        canvas.bind_all("<MouseWheel>", _on_mousewheel)
        
        self.create_script_buttons()
    
    def create_script_buttons(self):
        """Create compact buttons for each script with tooltips"""
        if not self.scripts_to_list:
            no_scripts_label = ttk.Label(self.scrollable_frame, 
                                       text="No automation scripts available", 
                                       font=("Segoe UI", 12), foreground="gray")
            no_scripts_label.pack(pady=50)
            return
        
        # Create a grid layout for buttons
        buttons_per_row = 2  # Adjust this based on your preference
        
        for i, script_info in enumerate(self.scripts_to_list):
            row = i // buttons_per_row
            col = i % buttons_per_row
            
            # Create button with clean, compact design
            button_text = f"🚀 {script_info.name}"
            
            run_btn = ttk.Button(self.scrollable_frame, 
                               text=button_text,
                               command=lambda s=script_info: self.run_script(s),
                               width=35,
                               style="Reset.TButton")  # Start with default style
            
            # Store button reference in script_info
            script_info.button = run_btn
            
            # Grid layout with padding
            run_btn.grid(row=row, column=col, padx=10, pady=8, sticky=(tk.W, tk.E))
            
            # Configure column weights for responsive layout
            self.scrollable_frame.columnconfigure(col, weight=1)
            
            # Create detailed tooltip
            tooltip_text = self.create_detailed_tooltip_text(script_info)
            self.create_tooltip(run_btn, tooltip_text)
    
    def create_detailed_tooltip_text(self, script_info):
        """Create detailed tooltip text for a script"""
        tooltip_lines = []
        tooltip_lines.append(f"📄 {script_info.name}")
        tooltip_lines.append("─" * (len(script_info.name) + 2))
        tooltip_lines.append("")
        
        # Show current status if available
        if script_info.execution_status:
            status_emoji = {
                "success": "✅",
                "failure": "❌", 
                "warning": "⚠️",
                "running": "🔄"
            }
            emoji = status_emoji.get(script_info.execution_status, "❓")
            tooltip_lines.append(f"🔍 Last Status: {emoji} {script_info.execution_status.upper()}")
            tooltip_lines.append("")
        
        tooltip_lines.append(f"📝 Description:")
        
        # Word wrap the description
        description = script_info.description
        words = description.split()
        lines = []
        current_line = []
        current_length = 0
        max_length = 60
        
        for word in words:
            if current_length + len(word) + 1 <= max_length:
                current_line.append(word)
                current_length += len(word) + 1
            else:
                if current_line:
                    lines.append(" ".join(current_line))
                current_line = [word]
                current_length = len(word)
        
        if current_line:
            lines.append(" ".join(current_line))
        
        for line in lines:
            tooltip_lines.append(f"   {line}")
        
        tooltip_lines.append("")
        tooltip_lines.append("🔄 Execution Flow:")
        tooltip_lines.append("   🔧 init_automation()    - Initialize resources")
        tooltip_lines.append("   ▶️ run_automation()     - Execute main logic")
        tooltip_lines.append("   🧹 cleanup_automation() - Clean up resources")
        tooltip_lines.append("")
        tooltip_lines.append("🎨 Visual Feedback:")
        tooltip_lines.append("   🔄 Yellow - Script running")
        tooltip_lines.append("   ✅ Green - Script succeeded")
        tooltip_lines.append("   ❌ Red - Script failed")
        tooltip_lines.append("")
        tooltip_lines.append("💡 Click to execute this automation script")
        
        return "\n".join(tooltip_lines)
    
    def create_tooltip(self, widget, text):
        """Create an enhanced tooltip for a widget"""
        def on_enter(event):
            # Create tooltip window
            tooltip = tk.Toplevel()
            tooltip.wm_overrideredirect(True)
            
            # Position tooltip
            x = event.x_root + 15
            y = event.y_root + 10
            
            # Ensure tooltip doesn't go off screen
            screen_width = tooltip.winfo_screenwidth()
            screen_height = tooltip.winfo_screenheight()
            
            # Estimate tooltip size (rough calculation)
            lines = text.split('\n')
            tooltip_width = max(len(line) for line in lines) * 8 + 20
            tooltip_height = len(lines) * 20 + 20
            
            if x + tooltip_width > screen_width:
                x = event.x_root - tooltip_width - 15
            if y + tooltip_height > screen_height:
                y = event.y_root - tooltip_height - 10
            
            tooltip.wm_geometry(f"+{x}+{y}")
            
            # Style the tooltip
            tooltip.configure(bg='#ffffcc', relief='solid', borderwidth=1)
            
            # Create label with the text
            label = tk.Label(tooltip, text=text, 
                           background="#ffffcc", 
                           foreground="black",
                           font=("Segoe UI", 9),
                           justify=tk.LEFT,
                           relief="flat",
                           padx=10, pady=8)
            label.pack()
            
            widget.tooltip = tooltip
        
        def on_leave(event):
            if hasattr(widget, 'tooltip'):
                widget.tooltip.destroy()
                del widget.tooltip
        
        widget.bind("<Enter>", on_enter)
        widget.bind("<Leave>", on_leave)
    
    def create_output_section(self, parent):
        output_frame = ttk.LabelFrame(parent, text="📋 Execution Log", padding="15")
        output_frame.grid(row=3, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        output_frame.columnconfigure(0, weight=1)
        output_frame.rowconfigure(0, weight=1)
        
        # Text widget with scrollbar
        text_frame = ttk.Frame(output_frame)
        text_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        text_frame.columnconfigure(0, weight=1)
        text_frame.rowconfigure(0, weight=1)
        
        self.output_text = tk.Text(text_frame, height=12, wrap=tk.WORD, 
                                  font=("Consolas", 10), bg="#1e1e1e", fg="#d4d4d4",
                                  insertbackground="white", selectbackground="#264f78")
        output_scrollbar = ttk.Scrollbar(text_frame, orient="vertical", 
                                       command=self.output_text.yview)
        self.output_text.configure(yscrollcommand=output_scrollbar.set)
        
        self.output_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        output_scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
    
    def log_output(self, message, level="INFO"):
        """Add message to output log with timestamp and color coding"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        
        # Level indicators
        level_indicators = {
            "INFO": "ℹ️",
            "SUCCESS": "✅", 
            "ERROR": "❌",
            "WARNING": "⚠️",
            "INIT": "🔧",
            "RUN": "▶️",
            "CLEANUP": "🧹",
            "EXIT": "🛑"
        }
        
        indicator = level_indicators.get(level, "📝")
        formatted_message = f"[{timestamp}] {indicator} {message}\n"
        
        self.output_text.insert(tk.END, formatted_message)
        self.output_text.see(tk.END)
        
        # Color coding
        start_line = self.output_text.index(tk.END + "-2l")
        end_line = tk.END + "-1c"
        
        colors = {
            "ERROR": "#ff6b6b",
            "SUCCESS": "#51cf66", 
            "WARNING": "#ffd43b",
            "INIT": "#74c0fc",
            "RUN": "#69db7c",
            "CLEANUP": "#ffa8a8",
            "EXIT": "#ff9f43"
        }
        
        if level in colors:
            tag_name = f"tag_{level.lower()}"
            self.output_text.tag_add(tag_name, start_line, end_line)
            self.output_text.tag_config(tag_name, foreground=colors[level])
        
        self.root.update_idletasks()
    
    def clear_output(self):
        """Clear the output log"""
        self.output_text.delete(1.0, tk.END)
        self.log_output("Output log cleared")
    
    def run_script(self, script_info):
        """Run a script with ScriptExitException handling and visual feedback"""
        def run_in_thread():
            try:
                # Set button to running state
                self.update_button_status(script_info, "running")
                
                self.log_output(f"Starting automation: {script_info.name}")
                self.log_output("─" * 50)
                
                # Execute the complete automation cycle
                results = script_info.execute_full_automation()
                
                self.log_output("─" * 50)
                
                # Determine success/failure and update button accordingly
                script_success = self.analyze_script_results(results, script_info)
                
                # Update button based on results
                if script_success:
                    self.update_button_status(script_info, "success")
                    self.log_output(f"✅ {script_info.name} completed successfully!", "SUCCESS")
                else:
                    # Check if it's a warning (graceful exit) or failure
                    if results.get('early_exit') and not self.has_critical_failure(results):
                        self.update_button_status(script_info, "warning")
                        self.log_output(f"⚠️ {script_info.name} completed with warnings", "WARNING")
                    else:
                        self.update_button_status(script_info, "failure")
                        self.log_output(f"❌ {script_info.name} failed", "ERROR")
                
            except Exception as e:
                # Update button to failure state
                self.update_button_status(script_info, "failure")
                error_msg = f"❌ Unexpected error in {script_info.name}: {str(e)}"
                self.log_output(error_msg, "ERROR")
                self.log_output(f"Traceback: {traceback.format_exc()}", "ERROR")
        
        # Run in separate thread to prevent UI freezing
        thread = threading.Thread(target=run_in_thread, daemon=True)
        thread.start()
    
    def analyze_script_results(self, results, script_info):
        """Analyze script results to determine overall success"""
        # Check if there was an early exit
        if results.get('early_exit'):
            self.log_output(f"Script exited early: {results.get('exit_message', 'Unknown reason')}", "EXIT")
            
            # Check if any phase returned False (critical failure)
            return not self.has_critical_failure(results)
        else:
            # Normal completion - check for any False results
            for phase, result in results.items():
                if phase not in ['early_exit', 'exit_message'] and result is False:
                    return False
            
            # Log results summary
            self.log_output("Execution Summary:", "SUCCESS")
            for phase, result in results.items():
                if phase not in ['early_exit', 'exit_message'] and result is not None:
                    if result is False:
                        self.log_output(f"  {phase.upper()}: FAILED", "ERROR")
                    elif isinstance(result, str):
                        self.log_output(f"  {phase.upper()}: {result}", "SUCCESS")
                    else:
                        self.log_output(f"  {phase.upper()}: {result}", "SUCCESS")
            
            return True
    
    def has_critical_failure(self, results):
        """Check if results contain any critical failures (False return values)"""
        for phase in ['init', 'run', 'cleanup']:
            if phase in results and results[phase] is False:
                return True
        return False
    
    def run_all_scripts(self):
        """Run all scripts sequentially with visual feedback"""
        def run_all_in_thread():
            self.log_output("🚀 Starting batch execution of all automation scripts...")
            self.log_output("═" * 60)
            
            total_scripts = len(self.scripts_to_list)
            successful = 0
            failed = 0
            warnings = 0
            
            for i, script_info in enumerate(self.scripts_to_list, 1):
                try:
                    # Set button to running state
                    self.update_button_status(script_info, "running")
                    
                    self.log_output(f"[{i}/{total_scripts}] Executing: {script_info.name}")
                    
                    # Execute the complete automation cycle
                    results = script_info.execute_full_automation()
                    
                    # Analyze results and update button
                    script_success = self.analyze_script_results(results, script_info)
                    
                    if script_success:
                        if results.get('early_exit') and not self.has_critical_failure(results):
                            warnings += 1
                            self.update_button_status(script_info, "warning")
                            self.log_output(f"⚠️ [{i}/{total_scripts}] {script_info.name} completed with warnings", "WARNING")
                        else:
                            successful += 1
                            self.update_button_status(script_info, "success")
                            self.log_output(f"✅ [{i}/{total_scripts}] {script_info.name} completed", "SUCCESS")
                    else:
                        failed += 1
                        self.update_button_status(script_info, "failure")
                        self.log_output(f"❌ [{i}/{total_scripts}] {script_info.name} failed", "ERROR")
                    
                    if i < total_scripts:
                        self.log_output("─" * 40)
                    
                except Exception as e:
                    failed += 1
                    self.update_button_status(script_info, "failure")
                    error_msg = f"❌ [{i}/{total_scripts}] Unexpected error in {script_info.name}: {str(e)}"
                    self.log_output(error_msg, "ERROR")
            
            self.log_output("═" * 60)
            self.log_output(f"🎉 Batch execution completed!", "SUCCESS")
            self.log_output(f"📊 Results: {successful} successful, {warnings} warnings, {failed} failed", "INFO")
        
        thread = threading.Thread(target=run_all_in_thread, daemon=True)
        thread.start()
    
    def show_script_info(self):
        """Show detailed information about all scripts"""
        info_window = tk.Toplevel(self.root)
        info_window.title("📋 Script Details")
        info_window.geometry("700x500")
        info_window.transient(self.root)
        info_window.configure(bg='#f0f0f0')
        
        # Main frame
        main_frame = ttk.Frame(info_window, padding="20")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Title
        title_label = ttk.Label(main_frame, text="📋 Automation Script Details", 
                               font=("Segoe UI", 16, "bold"))
        title_label.pack(pady=(0, 20))
        
        # Create text widget with scrollbar
        text_frame = ttk.Frame(main_frame)
        text_frame.pack(fill=tk.BOTH, expand=True)
        
        info_text = tk.Text(text_frame, wrap=tk.WORD, font=("Segoe UI", 11),
                           bg="white", fg="black", padx=15, pady=15)
        info_scrollbar = ttk.Scrollbar(text_frame, orient="vertical", command=info_text.yview)
        info_text.configure(yscrollcommand=info_scrollbar.set)
        
        info_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        info_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Generate script information
        info_content = ""
        
        for i, script_info in enumerate(self.scripts_to_list, 1):
            info_content += f"{i}. {script_info.name}\n"
            info_content += f"{'─' * (len(script_info.name) + 3)}\n\n"
            
            # Show current status if available
            if script_info.execution_status:
                status_display = {
                    "success": "✅ SUCCESS",
                    "failure": "❌ FAILED", 
                    "warning": "⚠️ WARNING",
                    "running": "🔄 RUNNING"
                }
                status_text = status_display.get(script_info.execution_status, "❓ UNKNOWN")
                info_content += f"🔍 Current Status: {status_text}\n\n"
            
            info_content += f"📝 Description:\n{script_info.description}\n\n"
            info_content += f"📦 Module: {script_info.module.__name__}\n\n"
            info_content += f"🔄 Execution Flow:\n"
            info_content += f"   🔧 init_automation()  - Initialize resources and setup\n"
            info_content += f"   ▶️ run_automation()   - Execute main automation logic\n"
            info_content += f"   🧹 cleanup_automation() - Clean up resources and finalize\n\n"
            info_content += f"🎨 Visual Status Indicators:\n"
            info_content += f"   🔄 Yellow - Script is currently running\n"
            info_content += f"   ✅ Green - Script completed successfully\n"
            info_content += f"   ⚠️ Red - Script had warnings (graceful early exit)\n"
            info_content += f"   ❌ Red - Script failed with errors\n\n"
            info_content += f"🛑 Exit Handling:\n"
            info_content += f"   • Supports ScriptExitException for graceful early exits\n"
            info_content += f"   • Can exit from any nested function or subprocess\n"
            info_content += f"   • Cleanup always runs even after early exit\n\n"
            
            # Check which functions are available
            available_functions = []
            for func_name in ['init_automation', 'run_automation', 'cleanup_automation']:
                if hasattr(script_info.module, func_name):
                    available_functions.append(f"✅ {func_name}")
                else:
                    available_functions.append(f"❌ {func_name}")
            
            info_content += f"🔍 Available Functions:\n"
            for func_status in available_functions:
                info_content += f"   {func_status}\n"
            
            if i < len(self.scripts_to_list):
                info_content += "\n" + "═" * 60 + "\n\n"
        
        info_text.insert(1.0, info_content)
        info_text.configure(state='disabled')
        
        # Close button
        close_btn = ttk.Button(main_frame, text="Close", command=info_window.destroy)
        close_btn.pack(pady=(15, 0))
    
    def run(self):
        """Start the application"""
        self.root.mainloop()

if __name__ == "__main__":
    app = ScriptManager()
    app.run()
