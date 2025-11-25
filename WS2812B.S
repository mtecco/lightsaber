; Assembly Code for WS2812B LED Control with Precise Timing

; Define special function register offset
#define __SFR_OFFSET 0x00
#include "avr/io.h"  ; AVR I/O definitions

; Declare global functions that can be called from C++ code
.global initWS2812B    ; Initialize LED control
.global sendColor      ; Send RGB color to one LED
.global clearLEDs      ; Turn off all LEDs
.global sendReset      ; Send reset signal to LEDs

; Initialize WS2812B Control
; Sets up the data pin as output and ensures it starts low
initWS2812B:
    SBI   DDRD, 3     ; Set Port D, Pin 3 as output (connected to DI of WS2812B)
    CBI   PORTD, 3    ; Set output low to start
    RET               ; Return to caller

; Send Reset Signal
; Sends 100+ μs low signal for reliable LED reset (longer than required 50μs)
sendReset:
    PUSH R16          ; Save R16 register
    PUSH R17          ; Save R17 register
    CBI   PORTD, 3    ; Set output low (start reset signal)

    ; Delay for 300μs (much longer than the required 50μs)
    ; At 16MHz: 300μs = 4800 cycles (300 * 16)
    LDI   R16, 240    ; Outer loop counter
    LDI   R17, 10     ; Inner loop counter (240 * 10 = 2400 cycles)
reset_outer:
reset_inner:
    NOP               ; No operation (1 cycle)
    NOP               ; No operation (1 cycle)
    NOP               ; No operation (1 cycle)
    NOP               ; No operation (1 cycle)
    DEC   R17         ; Decrement inner counter
    BRNE  reset_inner ; Branch to inner loop if not zero
    LDI   R17, 10     ; Reset inner counter
    DEC   R16         ; Decrement outer counter
    BRNE  reset_outer ; Branch to outer loop if not zero
    POP R17           ; Restore R17 register
    POP R16           ; Restore R16 register
    RET               ; Return to caller

; Send RGB Color to One LED
; Parameters: r24 (Red), r22 (Green), r20 (Blue)
; WS2812B expects data in GRB order (Green, Red, Blue)
sendColor:
    ; Save all registers that will be used
    PUSH R17
    PUSH R18
    PUSH R19
    PUSH R20
    PUSH R21
    PUSH R22
    PUSH R23
    PUSH R24
    PUSH R25
    PUSH R30
    ; Store color values in WS2812B expected order: Green, Red, Blue
    MOV R19, R22  ; Move Green to R19 (first to send)
    MOV R18, R24  ; Move Red to R18 (second to send)
    MOV R17, R20  ; Move Blue to R17 (third to send)
    ; Send all three bytes in correct GRB order
    MOV R24, R19    ; Load Green byte
    RCALL sendByte  ; Send Green
    MOV R24, R18    ; Load Red byte
    RCALL sendByte  ; Send Red
    MOV R24, R17    ; Load Blue byte
    RCALL sendByte  ; Send Blue
    ; Restore all saved registers
    POP R30
    POP R25
    POP R24
    POP R23
    POP R22
    POP R21
    POP R20
    POP R19
    POP R18
    POP R17
    RET  ; Return to caller

; Send a Single Byte to LEDs
; Parameter: r24 (byte to send)
; Implements precise timing for WS2812B protocol
; Revised sendByte function with more precise timing
sendByte:
    PUSH R25      ; Save R25 register
    PUSH R30      ; Save R30 register
    LDI   R25, 8  ; Set counter for 8 bits

; Main bit-sending loop - sends one byte bit by bit (MSB first)
sendBit:
    ; Check if current bit is 1 or 0
    LSL   R24     ; Logical Shift Left - MSB goes to carry flag
    BRCS  sendOne ; Branch if Carry Set (bit is 1) to sendOne

; Logic 0 code: T0H = 0.35μs ± 150ns, T0L = 0.9μs ± 150ns
; For 16MHz: 0.35us = ~5.6 cycles, 0.9us = ~14.4 cycles
; Actual: 5 cycles high (0.3125us), 14 cycles low (0.875us)
    SBI   PORTD, 3    ; High pulse start (2 cycles)
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle (total 5 cycles = 0.3125us)
    CBI   PORTD, 3    ; Low pulse start (2 cycles)
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle (total 14 cycles = 0.875us)
    RJMP  nextBit     ; Jump to next bit (2 cycles)

; Logic 1 code: T1H = 0.7μs ± 150ns, T1L = 0.6μs ± 150ns
; For 16MHz: 0.7us = ~11.2 cycles, 0.6us = ~9.6 cycles
; Actual: 11 cycles high (0.6875us), 9 cycles low (0.5625us)
sendOne:
    SBI   PORTD, 3    ; High pulse start (2 cycles)
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle (total 11 cycles = 0.6875us)
    CBI   PORTD, 3    ; Low pulse start (2 cycles)
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle
    NOP               ; 1 cycle (total 9 cycles = 0.5625us)

; Prepare for next bit
nextBit:
    DEC   R25         ; Decrement bit counter
    BRNE  sendBit     ; Branch if not all bits sent (back to sendBit)

    POP R30           ; Restore R30 register
    POP R25           ; Restore R25 register
    RET               ; Return to caller

; Clear All LEDs (Send Black/Off to All LEDs)
; Enhanced for reliability - clears more LEDs than needed and does multiple passes
; Uses external variables NUM_LEDS_LOW and NUM_LEDS_HIGH for LED count
clearLEDs:
    ; Save all registers that will be used
    PUSH R17
    PUSH R18
    PUSH R19
    PUSH R24
    PUSH R22
    PUSH R20
    PUSH R26
    PUSH R27
    PUSH R25
    ; Send initial reset to ensure clean start
    RCALL sendReset
    ; Load LED count from global variables (16-bit value)
    LDS R26, NUM_LEDS_LOW   ; Load low byte of LED count
    LDS R27, NUM_LEDS_HIGH  ; Load high byte of LED count
    ; Add extra LEDs to ensure complete clearing (safety margin)
    ADIW R26, 10   ; Add 10 extra LEDs to counter

; First pass - clear all LEDs plus extra safety margin
clearLoop:
    ; Check if we've cleared all LEDs (compare with zero)
    CP R26, R1       ; Compare low byte with 0 (R1 is always 0)
    CPC R27, R1      ; Compare high byte with carry
    BREQ clearPass2  ; Branch if equal (all LEDs cleared)
    ; Send black color (0,0,0) to current LED
    LDI R24, 0  ; Red = 0
    LDI R22, 0  ; Green = 0
    LDI R20, 0  ; Blue = 0
    RCALL sendColor  ; Send color data
    ; Decrement 16-bit counter
    SBIW R26, 1     ; Subtract immediate word (16-bit decrement)
    RJMP clearLoop  ; Repeat for next LED

; Second pass - clear again to ensure reliability
clearPass2:
    ; Send extra long reset between passes
    RCALL sendReset
    RCALL sendReset
    ; Reload original LED count for second pass
    LDS R26, NUM_LEDS_LOW   ; Reload low byte
    LDS R27, NUM_LEDS_HIGH  ; Reload high byte

clearLoop2:
    ; Check if we've cleared all LEDs in second pass
    CP R26, R1      ; Compare low byte with 0
    CPC R27, R1     ; Compare high byte with carry
    BREQ clearDone  ; Branch if equal (all LEDs cleared)
    ; Send black color again
    LDI R24, 0  ; Red = 0
    LDI R22, 0  ; Green = 0
    LDI R20, 0  ; Blue = 0
    RCALL sendColor  ; Send color data
    ; Decrement 16-bit counter
    SBIW R26, 1      ; Subtract immediate word
    RJMP clearLoop2  ; Repeat for next LED

; Final cleanup and reset
clearDone:
    ; Send multiple resets to ensure complete latching
    RCALL sendReset
    RCALL sendReset
    RCALL sendReset
    ; Restore all saved registers
    POP R25
    POP R27
    POP R26
    POP R20
    POP R22
    POP R24
    POP R19
    POP R18
    POP R17
    RET  ; Return to caller
