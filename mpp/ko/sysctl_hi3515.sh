#!/bin/sh

himm 0x20110104   0x9654;                       
himm 0x20110100   0x887;
himm 0x2011010c   0x3;     #ddr access priority                     
himm 0x20110110   0x10303; #arm9 D      
himm 0x20110114   0x10303; #arm9 I  
himm 0x20110118   0x7; 
himm 0x2011011c   0x7; 
himm 0x20110128   0x10020; #VO 
himm 0x2011012c   0x10601; #VI
#himm 0x20110130   0x5;     #TDE0, For better performance.
himm 0x20110130   0x10803;     #TDE0, To avoid VGA flicking.
himm 0x20110134   0x7;     
himm 0x20110138   0x10804; #VEDU0
himm 0x2011013c   0x7; 

himm 0x20050078   0x8002;   # vo bus timeout
#himm 0x20130000   0x020201; # vo outstanding, has configured in VO. For better performance.
#himm 0x101000F8   0x00010000; # vi outstanding, has configured in VI.
himm 0x20050054   0x1573;     # TDE frequency

himm 0x200f0094 0x1
himm 0x200f0098 0x1


