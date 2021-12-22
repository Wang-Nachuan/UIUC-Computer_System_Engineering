### P2 Solution

1.  
 - MTCP_BIOC_ON
   - When: whenever we need to consider the button input, i.e. user want enable the button.
   - Effect: Enable Button interrupt-on-change, which the change of button status will send interrupt to PC. 
   - Response: MTCP_ACK (successfully completes).
  
 - MTCP_LED_SET
   - When: whenever we need to set LEDs 
   - Effect: Set the User-set LED display values when the LED display is in USR mode.
   - Response: MTCP_ACK (successfully completes).
  
2.  
 - MTCP_ACK
   - When: In the document, response for MTCP_BIOC_ON, MTCP_BIOC_OFF, MTCP_DBG_OFF, MTCP_LED_SET.
   - Info: the MTC successfully completes a command.
  
 - MTCP_BIOC_EVENT
   - When:  MTCP_BIOC_EVT generate,  that is the Button Interrupt-on-change mode is enabled and a button is either pressed or released 
   - Info: Byte 0 for MTCP_BIOC_EVT packet, button states changes. And entire MTCP_BIOC_EVT indicates the button states details. 
  
 - MTCP_RESET
   - When: the device re-initializes itself
   - Info: the device just reset (for power-up, a RESET button press, or an MTCP_RESET_DEV command happened occur).
  
3.
 - The function is in the interrupt space, i.e. part of interrupt handler, it should run as fast as possible for safety and could not take much time, i.e. could not wait.
