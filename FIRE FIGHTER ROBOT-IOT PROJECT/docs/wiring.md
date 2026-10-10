# Fire-Fighting Robot — Wiring Guide

Step-by-step wiring instructions. Every connection listed wire-by-wire.
Pin numbers match `docs/hardware.md` and the firmware `#define` section.

> ⚠️ **Disconnect all power before wiring.** Double-check polarity before
> first power-on. GPIO 34/35 are input-only — never connect outputs there.

---

## 1. Tools & Materials Needed

| Item | Purpose |
|------|---------|
| Jumper wires (M-M, M-F) | Connections |
| Soldering iron + solder (or screw terminals) | Permanent joints |
| Multimeter | Voltage checks before power-on |
| Wire strippers / cutter | Prep |
| Heat shrink or electrical tape | Insulation |
| 1 kΩ resistor | Transistor base |
| 2N2222 NPN transistor | Pump switch |
| 1N4007 diode | Flyback protection |
| 470 µF capacitor (optional, recommended) | Motor noise filtering |

### Wire Color Convention (recommended)

| Color | Use |
|-------|-----|
| Red | +5 V / VCC |
| Black | GND |
| Yellow / Orange | Sensor signal (DO) |
| Blue / Green | Motor control (IN1–IN4) |
| White / Grey | PWM (ENA/ENB) |
| Purple | Pump control |

---

## 2. Power Distribution (Wire This FIRST)

```
Battery (7.4 V 2S)
   │
   ├─ RED +  ──► L298N +12V (VMS) terminal
   └─ BLK –  ──► L298N GND terminal
                    │
                    └─ L298N GND ──► ESP32 GND  ← CRITICAL LINK
```

| # | From | To | Wire |
|---|------|----|------|
| P1 | Battery + | L298N **+12V (VMS)** | Red, thick (18–20 AWG) |
| P2 | Battery – | L298N **GND** | Black, thick |
| P3 | L298N **5V** terminal | ESP32 **VIN** | Red |
| P4 | L298N **GND** | ESP32 **GND** | Black |
| P5 | L298N **5V** terminal | Sensor power rail (breadboard red line) | Red |
| P6 | L298N **GND** | Sensor ground rail (breadboard blue line) | Black |

> ✅ Keep the **5V jumper on L298N ON** (battery ≤ 12 V) — its onboard
> regulator powers the ESP32 and sensors.
> 🔴 **P4 is the most important wire in the whole robot.** No common ground
> = sensor readings garbage + motors behave randomly.

---

## 3. Flame Sensors (×3)

All three sensors share power. DO (digital out) goes to its own GPIO.

| # | From | To | Wire |
|---|------|----|------|
| F1 | Left sensor **VCC** | 5 V rail | Red |
| F2 | Left sensor **GND** | GND rail | Black |
| F3 | Left sensor **DO** | **GPIO 34** | Yellow |
| F4 | Right sensor **VCC** | 5 V rail | Red |
| F5 | Right sensor **GND** | GND rail | Black |
| F6 | Right sensor **DO** | **GPIO 35** | Yellow |
| F7 | Center sensor **VCC** | 5 V rail | Red |
| F8 | Center sensor **GND** | GND rail | Black |
| F9 | Center sensor **DO** | **GPIO 32** | Yellow |

### Mounting
- Sensors on the **front edge** of the chassis, angled slightly outward:
  left and right at ~30°, center facing straight ahead.
- Same height (~2–5 cm above floor), pointing at flame level.
- Turn each module's **potentiometer** so it triggers at ~0.8–1 m from a
  candle flame (test one at a time before final mount).

---

## 4. L298N ↔ ESP32 (Control Signals)

| # | From ESP32 | To L298N | Function |
|---|-----------|----------|----------|
| M1 | **GPIO 26** | **IN1** | Left motor direction A |
| M2 | **GPIO 27** | **IN2** | Left motor direction B |
| M3 | **GPIO 14** | **IN3** | Right motor direction A |
| M4 | **GPIO 13** | **IN4** | Right motor direction B |
| M5 | **GPIO 25** | **ENA** | Left motor speed (PWM) |
| M6 | **GPIO 33** | **ENB** | Right motor speed (PWM) |

> If your L298N has ENA/ENB jumpers: **remove the jumpers** — we drive
> speed with PWM from the ESP32.

---

## 5. Motors ↔ L298N

| # | From | To |
|---|------|----|
| D1 | Left motor wire 1 | L298N **OUT1** |
| D2 | Left motor wire 2 | L298N **OUT2** |
| D3 | Right motor wire 1 | L298N **OUT3** |
| D4 | Right motor wire 2 | L298N **OUT4** |

### Direction sanity check (after firmware upload)
| Command | Expected | If wrong |
|---------|----------|----------|
| Forward | Both wheels spin forward | Swap BOTH wires of the wrong motor |
| Turn left | Left wheel stops/reverses, right spins forward | Swap that motor's wires |
| Turn right | Right wheel stops/reverses, left spins forward | Swap that motor's wires |

> 💡 Don't solder motor wires yet — you may need to swap them.

---

## 6. Water Pump Circuit (Transistor Switch)

GPIO 23 can't drive the pump directly — it can't supply enough current.
The transistor does the switching:

```
                 +5 V
                   │
              ┌────┴────┐
              │  PUMP + │
              └────┬────┘
                   │
              ┌────┴────┐
              │  PUMP – │
                   │
            Collector (C)
             2N2222 NPN
            Base (B) ────[ 1 kΩ ]──── GPIO 23
            Emitter (E)
                   │
                  GND
```

| # | From | To | Wire |
|---|------|----|------|
| S1 | 5 V rail | Pump **+** terminal | Red |
| S2 | Pump **–** terminal | 2N2222 **Collector** | Black |
| S3 | 2N2222 **Emitter** | GND rail | Black |
| S4 | **GPIO 23** | 1 kΩ resistor → 2N2222 **Base** | Purple |
| S5 | 1N4007 diode **stripe (cathode)** | Pump **+** terminal | — |
| S6 | 1N4007 diode **other end (anode)** | Pump **–** terminal | — |

**Transistor pinout (2N2222, flat face toward you, legs down):**
`E │ B │ C`  →  Emitter, Base, Collector (left to right)
> ⚠️ Pinout varies by manufacturer — verify with the datasheet.

**Diode:** stripe must face **PUMP +** (blocks reverse voltage spike).

### Spray head
- Silicone/food tube from pump outlet → lightweight nozzle
- Mount nozzle on front of chassis, aimed slightly downward-forward
- Submerge pump inlet in a small water bottle/tank mounted on chassis

---

## 7. Full System Diagram

```
                         ┌──────────────────┐
   Flame L ──DO── GPIO 34│                  │GPIO 26──IN1┐
   Flame R ──DO── GPIO 35│     ESP32        │GPIO 27──IN2│ ┌────────┐
   Flame C ──DO── GPIO 32│                  │GPIO 14──IN3├─┤  L298N │──OUT1/2── Left motor
        5V ──────────────┤  (VIN, GND)      │GPIO 13──IN4│ │        │──OUT3/4── Right motor
       GND ──────────────┤                  │GPIO 25──ENA│ │        │
                         └──────┬───────────┘GPIO 33──ENB┘ └────┬───┘
                                │ GPIO 23                        │5V  │GND
                          [1 kΩ]│                               │    │
                             2N2222 Base                     Battery  ESP32
                          Pump via C/E                        VMS/GND  VIN/GND
```

---

## 8. Pre-Power Checks (Multimeter — DO NOT SKIP)

| Check | Where | Expected |
|-------|-------|----------|
| Battery voltage | Battery terminals | 7.4 V ± 0.3 V |
| L298N 5V output | 5V ↔ GND terminals | 4.9–5.1 V |
| No short on 5V rail | Resistance 5V↔GND (power OFF) | Not near 0 Ω |
| No short on battery | Resistance VMS↔GND (power OFF) | Not near 0 Ω |
| Diode direction | Diode across pump | Stripe toward + |

**Only after all 5 pass → connect power.**

---

## 9. First Power-On Sequence

1. **Motors disconnected** — power up. ESP32 boot LED on, no smoke/heat.
2. Open Serial Monitor @ **115200** — confirm sensor readings print.
3. Wave a lit match ~30 cm in front of each sensor — verify the
   corresponding sensor flags.
4. **Power off**, connect motor wires (DON'T mount pump in water yet).
5. Power on — test Forward / Left / Right commands.
6. Swap motor wires per section 5 table if any direction is wrong.
7. Test pump: trigger manually — confirm spray + **no ESP32 reset**.
8. If ESP32 resets when pump starts → pump shares weak supply; power pump
   from a separate 5 V source (common GND still required).

---

## 10. Common Wiring Mistakes

| Mistake | Symptom | Fix |
|---------|---------|-----|
| Missing common ground | Random readings, motors twitch | Wire P4 (L298N GND → ESP32 GND) |
| Signal into GPIO 34/35 from output | Pin dead | These are input-only — sensors only |
| ENA/ENB jumpers left on | Motors full-speed, no PWM control | Remove jumpers |
| Diode backwards | Transistor dies / pump erratic | Stripe toward pump + |
| Sensor pot fully one way | Always LOW or never LOW | Adjust potentiometer mid-range |
| Motor wires to wrong OUT pair | Wheels spin opposite | Swap that motor's two wires |
| Pump on ESP32 5V pin | Brownout / reset on spray | Pump on 5 V rail, transistor-switched |

---

## 11. Final Wiring Checklist

- [ ] P1–P6 power wires done, common ground confirmed
- [ ] 3 flame sensors: VCC/GND/DO wired, pots adjusted
- [ ] L298N: IN1–IN4, ENA/ENB connected; EN

- [ ] L298N: IN1–IN4, ENA/ENB connected; ENA/ENB jumpers removed
- [ ] Motors on OUT1/2 and OUT3/4, directions verified
- [ ] Pump circuit: transistor (E-B-C correct), 1 kΩ base resistor, diode stripe toward pump +
- [ ] Spray nozzle mounted and aimed, pump inlet submerged
- [ ] All 5 multimeter pre-power checks passed
- [ ] First power-on done with motors disconnected
- [ ] Sensor response verified with controlled flame
- [ ] No ESP32 reset during motor/pump operation.
