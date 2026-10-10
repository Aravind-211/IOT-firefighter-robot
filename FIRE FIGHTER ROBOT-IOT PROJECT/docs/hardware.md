# Fire-Fighting Robot — Hardware Documentation

A simple, efficient autonomous fire-fighting robot built on ESP32.
Flame sensors detect fire direction → robot moves toward it → water pump
sprays to extinguish → done.

**Detect → Move → Spray.** Nothing more.

---

## 1. Components (BOM)

| # | Component | Qty | Purpose |
|---|-----------|-----|---------|
| 1 | ESP32 DevKit (WROOM-32) | 1 | Main controller |
| 2 | Flame sensor module (IR) | 3 | Left / Center / Right fire detection |
| 3 | L298N motor driver board | 1 | Drives the DC motors |
| 4 | DC gear motors + wheels | 2 | Robot movement (differential drive) |
| 5 | Water pump (5 V DC mini pump) | 1 | Water spray mechanism |
| 6 | NPN transistor (2N2222) or MOSFET module | 1 | Switches the pump |
| 7 | Diode 1N4007 | 1 | Flyback protection across pump |
| 8 | Battery (7.4 V Li-ion 2S) | 1 | Main power |
| 9 | Chassis + wires | 1 set | Assembly |

---

## 2. Pin Map

| Function | GPIO | Type |
|----------|------|------|
| Flame sensor LEFT | **GPIO 34** | Input (input-only pin) |
| Flame sensor RIGHT | **GPIO 35** | Input (input-only pin) |
| Flame sensor CENTER | **GPIO 32** | Input |
| L298N IN1 (Left motor) | **GPIO 26** | Output |
| L298N IN2 (Left motor) | **GPIO 27** | Output |
| L298N IN3 (Right motor) | **GPIO 14** | Output |
| L298N IN4 (Right motor) | **GPIO 13** | Output |
| L298N ENA (Left speed) | **GPIO 25** | PWM |
| L298N ENB (Right speed) | **GPIO 33** | PWM |
| Water pump | **GPIO 23** | Output (via transistor) |

> ⚠️ If your firmware `.ino` uses different pins, update this table to match.
> The `#define` section in the code is the source of truth.

**Why these pins?**
- GPIO 34/35 are input-only → perfect for flame sensor digital outputs.
- GPIO 25/33 support PWM → motor speed control via `ledcWrite()`.
- No ADC2 pins used → no conflict with Wi-Fi.

---

## 3. Wiring

### Flame Sensors (×3)
```
Sensor VCC → 5 V      Sensor GND → GND      Sensor DO → GPIO (34/35/32)
```

### L298N + Motors
```
Battery +  → L298N VMS
Battery –  → L298N GND  (must connect to ESP32 GND too)
Left motor  → OUT1 / OUT2
Right motor → OUT3 / OUT4
IN1–IN4     → GPIO 26 / 27 / 14 / 13
ENA / ENB   → GPIO 25 / 33 (PWM)
5V jumper   → ON (if battery ≤ 12 V)
```

### Water Spray Mechanism
```
GPIO 23 → 1 kΩ resistor → transistor base (2N2222)
Pump +  → 5 V
Pump –  → transistor collector
Emitter → GND
1N4007 diode across pump terminals (stripe toward +)
```
Spray head (small nozzle/tube) mounted on the chassis, pointing forward.

---

## 4. Power

```
Battery 7.4 V ──► L298N VMS (motors)
              └──► L298N 5V regulator ──► ESP32 VIN + sensors + pump logic
```

**Rules:**
- All grounds connected to ONE common point.
- Never power motors from ESP32 pins.
- ESP32 gets power from L298N's onboard 5 V or a buck converter.

---

## 5. How It Works (Logic)

1. Read the 3 flame sensors.
2. Left sensor LOW → turn left. Right sensor LOW → turn right.
   Center sensor LOW → move forward.
3. Fire close (center triggered + threshold) → **stop + pump ON** (spray).
4. Fire out / sensors HIGH → pump OFF.

---

## 6. Testing Checklist

- [ ] Serial monitor (115200) prints sensor states correctly
- [ ] Cover left sensor → robot turns left ✓
- [ ] Cover right sensor → robot turns right ✓
- [ ] Both sensors clear → robot stops ✓
- [ ] Pump switches ON/OFF cleanly with no ESP32 reset
- [ ] Spray reaches the target area (~0.5–1 m)

---

## 7. Troubleshooting

| Symptom | Fix |
|---------|-----|
| ESP32 resets when motors run | Common ground missing / motor noise → add 470 µF cap near L298N |
| Flame sensor always LOW | Sunlight interference → shield it, adjust potentiometer |
| Pump doesn't turn on | Check transistor orientation + diode direction |
| One motor doesn't spin | Check that motor's OUT wires + IN pins |
| Robot turns wrong direction | Swap that motor's OUT1/OUT2 wires |

---

## 8. Safety

⚠️ **Educational project only — not a certified fire-protection system.**
Test only with small controlled flames (candle/match) in a safe area, with
water and supervision available.

---

*Documentation v1.0 — Fire-Fighting Robot*
