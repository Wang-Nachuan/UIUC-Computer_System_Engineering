### P1 Solution

1. 
- How VGA split screen
   - VGA provides the ability to specify a horizontal division which divides the screen into two windows which can start at separate display memory addresses. To achieve the screen split, we need to use Line Compare Field. 
   - The Line Compare field specifies the scan line at which a horizontal division can occur, providing for split-screen operation. 
   - When the scan line counter reaches the value in the Line Compare field, the current scan line address is reset to 0 and the Preset Row Scan is presumed to be 0. If the Pixel Panning Mode field is set to 1 then the Pixel Shift Count and Byte Panning fields are reset to 0 for the remainder of the display cycle.
 -  How the VGA acts upon them:
    - The VAG works as what it should be without spliting before the "line counter" reaches the 
    value in the Line Compare Field. 
    - After it reaches the line, the current scan line start address is reset to 0.
 - Settings
   - Turn off Splitting - If no horizontal division is required, Line Compare field should be set to 3FFh (all 10 bits to 1). 
   - Turn On Splitting - Set start address of scan line before split operation. 
   - Disable the panning function of the bottom division (as status bar) - set Pixel Panning Mode (bit 5 of Attribute Mode Control Register) to 0. Now that the bottom division is fixed, we may control the panning and scrolling of the top division by Pixel Shift Count field, Byte Panning field. Additional pixel-level scrolling can be controlled by Preset Row Scan in which the maximum value is defined in Maximum Scan Line.
 - Further Constraints
   - Cancel CRTC Registers protection manually before setting splitting operation via the Enable Vertical Retrace Access and CRTC Registers Protect Enable fields. 
   - The bottom window's starting display memory address is fixed at 0. This means that the bottom screen must be located first in memory and followed by the top
   - If the Pixel Panning Mode field is set to 1 then the Pixel Shift Count and Byte Panning fields are reset to 0 for the remainder of the display cycle allowing the top window to pan while the bottom window remains fixed. Otherwise, both windows pan by the same amount.
   - The hardware does not provide for split-screen modes where multiple video modes are possible in one display screen as provided by some non-VGA graphics controllers. 
   - Either both windows are panned by the same amount, or only the top window pans, in which case, the bottom window's panning values are fixed at 0. And the Preset Row Scan field only applies to the top window.
   

1.  
- Change Palette is to write Palette RAM. 
  - For a palette entry, output the palette entry's index value to the DAC Address Write Mode Register, then perform 3 writes to the DAC Data Register, loading R, G, B into the palette RAM. The internal write address will auto increment, allowing the writing to the next address without reprogramming DAC Address Write Mode Register.
  - Sequence
    Step1: DAC Address Write Mode Register (3C8h)<-- palette entry's index value
    Step2: DAC Data Register (3C9h) <-- to-write Red value 
    Step3: DAC Data Register (3C9h) <-- to-write Green value 
    Step4: DAC Data Register (3C9h) <-- to-write Blue value
  - When displaying, it read a palette entry. Output the palette entry's index to the DAC Address Read Mode Register. Then perform 3 reads from the DAC Data Register, loading R, G, B values from palette RAM, similarly to writing.
  


- Specific Sequence Example:
```
    # Save DAC State Register (port 3C7h);
    movw    0x3c7h, %dx
    in      %dx, %ax
    pushw   %ax

    # Save PEL Address Write Mode Register (port 3C8h);
    movw    0x3c8h, %dx
    in      %dx, %ax
    pushw   %ax

    # Write the value of the color entry to PEL Address Write Mode Register (port 3C8h);
    movw    /* Memory address of color entry*/, %ax
    out     %ax, %dx

    # Write the RGB values to the PEL Data Register (port 3C9h)
    movw    0x3c9h, %dx
    movw    /* red component value */, %ax
    out     %ax, %dx
    movw    /* blue component value */, %ax
    out     %ax, %dx
    movw    /* green component value */, %ax
    out     %ax, %dx

    # Restore previouse values for DAC State Register and PEL Address Write Mode Register
    movw    0x3c8h, %dx
    popw    %ax
    out     %ax, %dx
    movw    0x3c7h, %dx
    popw    %ax
    out     %ax, %dx
```



 
 
