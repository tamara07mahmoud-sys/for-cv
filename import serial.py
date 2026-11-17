import serial
import time
import matplotlib.pyplot as plt
import random

# --- 1. Setup Connection ---
try:
    ser = serial.Serial('COM3', 9600, timeout=1)
    time.sleep(2)
    print("✅ Connected to Arduino")
except:
    print("⚠️ Simulation Mode (No Arduino detected)")
    ser = None

# --- 2. Setup Graphics Interface ---
plt.ion()
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 10))
temp_h, co2_h, sound_h, times = [], [], [], []

def send_led_command(command):
    """Send command to Arduino and print it on the screen to monitor"""
    # Always print the command in the Terminal to ensure the system is working
    print(f"💡 [LED COMMAND]: {command}")
    
    # If Arduino is connected, send the command over the serial cable
    if ser:
        ser.write(f"{command}\n".encode())

# --- 3. Main Operational Loop ---
print("🚀 System Running... Monitoring Sensors & Controlling LEDs\n")
count = 0

try:
    while True:
        # 1. Fetch data (Real or Simulated)
        if ser and ser.in_waiting > 0:
            try:
                raw = ser.readline().decode('utf-8').strip()
                parts = raw.split(',')
                data = {"temp": float(parts[0]), "co2": int(parts[1]), "vib": int(parts[2]), "snd": int(parts[3])}
            except: continue
        else:
            # Random data for simulation
            data = {"temp": round(random.uniform(33, 39), 1), "co2": random.randint(400, 900), "vib": random.choice([0,1]), "snd": random.randint(20, 100)}

        count += 1

        # 2. Print readings in the Terminal
        print("-" * 45)
        print(f"📊 Sensor Readings (Update {count}):")
        print(f"🌡️ Temp: {data['temp']} °C | 💨 Gas: {data['co2']} ppm | 🔊 Sound: {data['snd']} % | 📳 Vibration: {data['vib']}")

        # 3. Make decisions and send/print LED commands
        if data['temp'] > 37.0:
            send_led_command("LED_TEMP_ON")
        else:
            send_led_command("LED_TEMP_OFF")

        if data['co2'] > 700:
            send_led_command("LED_CO2_ON")
        else:
            send_led_command("LED_CO2_OFF")

        if data['vib'] == 1 or data['snd'] > 60:
            send_led_command("LED_ALERT_ON")
        else:
            send_led_command("LED_ALERT_OFF")

        # 4. Update the plots
        times.append(count)
        temp_h.append(data['temp'])
        co2_h.append(data['co2'])
        sound_h.append(data['snd'])

        if len(times) > 20:
            times.pop(0); temp_h.pop(0); co2_h.pop(0); sound_h.pop(0)

        ax1.cla(); ax2.cla(); ax3.cla()
        
        ax1.plot(times, temp_h, 'r-o')
        ax1.set_title(f"Temperature: {data['temp']} °C", fontsize=12, color='red')
        ax1.set_ylim(25, 45) # Fix the axis limit to make the plot clearer
        
        ax2.plot(times, co2_h, 'g-s')
        ax2.set_title(f"CO2 Level: {data['co2']} ppm", fontsize=12, color='green')
        ax2.set_ylim(300, 1000)
        
        ax3.plot(times, sound_h, 'b-^')
        ax3.set_title(f"Sound Level: {data['snd']} %", fontsize=12, color='blue')
        ax3.set_ylim(0, 120)
        
        plt.tight_layout()
        plt.pause(1.0) # 1-second delay so you can read the screen comfortably

except KeyboardInterrupt:
    print("\n🛑 Shutting down system...")
    if ser: ser.close()