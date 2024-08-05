# Hardware
The ESP_MKB hardware and PCB design were done in EasyEDA Pro.
<a href="https://oshwlab.com/bart_1855/macrokb">Link to the project page</a>

# MCP73871 settings
the battery charge controller IC has a couple of settings which need to be set to the correct value or reference voltage to maken sure the IC works es intended for the circuit it is installed in.
I have made my choices regarding these settings according to the datasheet and some online sources.
The settings for Version 1.0 of the ESP_MKB are as follows:
<table>
	<tr>
		<th>Setting</th>
		<th>Pin</th>
		<th>Description</th>
		<th>Choice</th>
		<th>Notes</th>
	</tr>
	<tr>
		<td>SEL</td>
		<td>3</td>
		<td>POWER SOURCE SELECT (H=AC-DC, L=USB)</td>
		<td>Low (GND)</td>
		<td>Only USB is used</td>
	</tr>
	<tr>
		<td>PROG1</td>
		<td>13</td>
		<td>Fast charge current regulation setting with SEL = high.</td>
		<td>2K TO GND (500mA)</td>
		<td>Unsure if this setting is ignored. used 2K Ohm to set to 500mA anyway</td>
	</tr>
	<tr>
		<td>PROG2</td>
		<td>4</td>
		<td>USB input current limit selection (L=100mA, H = 500 mA)</td>
		<td>High (Vin)</td>
		<td>500mA is the target over USBC</td>
	</tr>
	<tr>
		<td>PROG3</td>
		<td>12</td>
		<td>Termination set point for USB port</td>
		<td>100K Ohm to GND</td>
		<td>Stop charging when charge current is 10mA</td>
	</tr>
	<tr>
		<td>TE</td>
		<td>9</td>
		<td>Timer Enable; Enables Safety Timer when active-low</td>
		<td>High (Vin)</td>
		<td>Timer is disabled in stock board, but solderjumper is availabe to enable it after testing</td>
	</tr>
	<tr>
		<td>CE</td>
		<td>17</td>
		<td>Device Charge Enable; Enabled when CE = high</td>
		<td>High (Vin)</td>
		<td>Charging is enabled whn USB is connected</td>
	</tr>
	<tr>
		<td>VPCC</td>
		<td>2</td>
		<td>Voltage Proportional Current Control</td>
		<td>Vvpcc=1.23v, R1=270K, R2=100K</td>
		<td>If USB voltage is lower than expected, slow down charging, stop at 4.5v</td>
	</tr>
</table>

#BOM
<table border="1" cellpadding="0" cellspacing="0">
	<tbody>
		<tr>
			<td>Designator</td>
			<td>Quantity</td>
			<td>Manufacturer</td>
			<td>Manufacturer Part</td>
			<td>Value</td>
			<td>Datasheet</td>
			<td>Extra info</td>
		</tr>
		<tr>
			<td>BM1,BM2,BM3,BM4,BM11,BM12,BM13,BM14</td>
			<td>8</td>
			<td>ANY</td>
			<td>CHERRY MX SWITCH</td>
			<td>&nbsp;</td>
			<td>&nbsp;</td>
			<td>NOT PREDEFINED. FOR PERSONAL CHOICE</td>
		</tr>
		<tr>
			<td>BM21,BM22,BM23,BM24</td>
			<td>4</td>
			<td>XUNPU</td>
			<td>TS-1002S-04326C</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20191118/C455126_F61E8FA1ED640FB1CE0B30B55DEB62E3.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>BT1</td>
			<td>1</td>
			<td>MYOUNG</td>
			<td>BH-18650-B6AA002</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20230821/992B872406559792B6674E1E1C6C3ECD.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>C1</td>
			<td>1</td>
			<td>KNSCHA</td>
			<td>01EC4294SHC1000UF25V</td>
			<td>1000uF</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20200324/C503217_05464B9B8F3E25C695CF18DE02B15AC9.pdf">Datasheet</a></td>
			<td>VALUE NOT FINAL</td>
		</tr>
		<tr>
			<td>C2,C4,C5,C15,C21,C23</td>
			<td>6</td>
			<td>SAMSUNG</td>
			<td>CL10A105KO8NNNC</td>
			<td>1uF</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20220919/9FBE606BE0D9532D730F292EB26DC5BB.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>C3,C6,C7,C14,C19</td>
			<td>5</td>
			<td>SAMSUNG</td>
			<td>CL10A226MQ8NRNC</td>
			<td>22uF</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20160218/1457707763339.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>C8,C9,C18,C25,C27,C29</td>
			<td>6</td>
			<td>SAMSUNG</td>
			<td>CL10B104KB8NNNC</td>
			<td>100nF</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20160218/1457707763339.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>C10,C11,C12,C13,C30,C31,C32</td>
			<td>7</td>
			<td>SAMSUNG</td>
			<td>CL10A106MQ8NNNC</td>
			<td>10uF</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20160218/1457707763339.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>C20,C22,C24,C26,C28</td>
			<td>5</td>
			<td>YAGEO</td>
			<td>CC0603JRNPO9BN101</td>
			<td>100pF</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20181008/C309499_21CC21451B82C39E3D92150FD6B30293.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>D20</td>
			<td>1</td>
			<td>FUXINSEMI</td>
			<td>PRTR5V0U2X</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20220516/33864E07948E6E84F86E9BAB585532F5.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>D21</td>
			<td>1</td>
			<td>DIODES</td>
			<td>ZLLS410TA</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20200421/C177096_D8075DEB85C058F50C34281A46D0D7C3.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>F1</td>
			<td>1</td>
			<td>BOURNS</td>
			<td>MF-PSMF075X-2</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20170704/1499151353027.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>L1,L2</td>
			<td>2</td>
			<td>muRata</td>
			<td>DFE252012F-2R2M=P2</td>
			<td>2.2uH</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20200716/C703141_C4AFEA7D31E0931E23F948A8B91A9E18.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>LED1,LED2,LED3,LED4,LED5,LED6,LED7,LED8,LED10,LED11,LED12</td>
			<td>11</td>
			<td>XINGLIGHT</td>
			<td>XL-1615RGBC-WS2812B</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20240218/3051C7463171A0558E4806C93ACA0B19.pdf">Datasheet</a></td>
			<td>TOP SIDE LEDS</td>
		</tr>
		<tr>
			<td>LED9,LED13,LED14,LED15,LED16,LED17,LED18</td>
			<td>7</td>
			<td>XINGLIGHT</td>
			<td>XL-5050RGBC-WS2812B</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20240412/E1004CAA80592E31D08BFE019083806F.pdf">Datasheet</a></td>
			<td>BOTTOM SIDE LEDS</td>
		</tr>
		<tr>
			<td>NUT1,NUT2,NUT3,NUT4</td>
			<td>4</td>
			<td>Sinhoo</td>
			<td>SMTSO3035CTJ</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20211122/17CF5E24EB4D4DC090A80DE3D7DE5E21.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>R1,R2</td>
			<td>2</td>
			<td>UNI-ROYAL</td>
			<td>0603WAF5101T5E</td>
			<td>5.1k</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20200306/C422600_1E6D84923E4A46A82E41ADD87F860B5C.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>R3,R30,R31,R32</td>
			<td>4</td>
			<td>UNI-ROYAL</td>
			<td>0603WAF2001T5E</td>
			<td>2k</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20200306/C422600_1E6D84923E4A46A82E41ADD87F860B5C.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>R4,R6</td>
			<td>2</td>
			<td>YAGEO</td>
			<td>RC0603FR-07100KL</td>
			<td>100k</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20140324/1457707028752.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>R5</td>
			<td>1</td>
			<td>UNI-ROYAL</td>
			<td>0603WAF2703T5E</td>
			<td>270k</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20200306/C422600_1E6D84923E4A46A82E41ADD87F860B5C.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>R7</td>
			<td>1</td>
			<td>Milliohm</td>
			<td>HoYH0603-1/2W-10mR-1%-TCR50</td>
			<td>10m</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20211116/A510C0D8A660B2C11D153FDB54B68FB7.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>R8</td>
			<td>1</td>
			<td>UNI-ROYAL</td>
			<td>0603WAF510KT5E</td>
			<td>5.1</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20200306/C422600_1E6D84923E4A46A82E41ADD87F860B5C.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>R9,R10,R11,R12</td>
			<td>4</td>
			<td>UNI-ROYAL</td>
			<td>0603WAF510JT5E</td>
			<td>51</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20200306/C422600_1E6D84923E4A46A82E41ADD87F860B5C.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>R25,R26</td>
			<td>2</td>
			<td>ALPSALPINE</td>
			<td>RS08U111Z001</td>
			<td>10k</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20180403/C202411_1E02FBA416091575CFC251DF035433A7.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>RN1,RN2,RN3,RN4,RN5,RN6,RN7</td>
			<td>7</td>
			<td>UNI-ROYAL</td>
			<td>4D03WGJ0103T5E</td>
			<td>10k</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20191224/C474318_2527498AE30E813171187CCD28401A71.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>SIGNALLED1,SIGNALLED2,SIGNALLED3</td>
			<td>3</td>
			<td>XINGLIGHT</td>
			<td>XL-1608UBC-04</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20240218/F76B043D1CDACE9DB8357580572EF0D5.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>ST1,ST2,ST3,ST4,ST11,ST12,ST13,ST14</td>
			<td>8</td>
			<td>Kailh</td>
			<td>CPG151101S11-16</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20220829/33DFC1A320A078D20879F2B21562B513.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>SW1</td>
			<td>1</td>
			<td>SHOU HAN</td>
			<td>MSK12C02</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20190925/C431540_89155AEF0758B9AD1B2D805F83FB12E3.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>SW3,SW4</td>
			<td>2</td>
			<td>BZCN</td>
			<td>TSB003A3518A03</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20210906/5AB5E3D422037A39DE138E991ECE43CA.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>THERM1</td>
			<td>1</td>
			<td>muRata</td>
			<td>NCP18XH103F03RB</td>
			<td>10K(1002)</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20130819/1457706638081.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>U0</td>
			<td>1</td>
			<td>ESPRESSIF</td>
			<td>ESP32-C6-MINI-1-N4</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20240130/4529D418EFEC832950EB8101798FAED6.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>U1</td>
			<td>1</td>
			<td>MICROCHIP</td>
			<td>MCP73871T-2CCI/ML</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20200408/C511310_EA348E8D1D9598593724D444DBAC01FF.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>U2</td>
			<td>1</td>
			<td>TI</td>
			<td>BQ27441DRZR-G1B</td>
			<td>&nbsp;</td>
			<td><a href="https://www.ti.com/cn/lit/gpn/bq27441-g1">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>U3</td>
			<td>1</td>
			<td>TI</td>
			<td>TPS63051RMWR</td>
			<td>&nbsp;</td>
			<td><a href="https://www.ti.com/cn/lit/gpn/tps63051">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>U4</td>
			<td>1</td>
			<td>TI</td>
			<td>TPS613222ADBVR</td>
			<td>&nbsp;</td>
			<td><a href="https://www.ti.com/cn/lit/gpn/tps61322">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>U5</td>
			<td>1</td>
			<td>TI</td>
			<td>TCA8418RTWR</td>
			<td>&nbsp;</td>
			<td><a href="https://www.ti.com/cn/lit/gpn/tca8418">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>U6</td>
			<td>1</td>
			<td>ALPSALPINE</td>
			<td>EC12E24204A2</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20190114/C351238_03113AE9177DD7D8BA81ABE0CC2A5D0F.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>U7,U8</td>
			<td>2</td>
			<td>KNOWLES</td>
			<td>SPH0645LM4H-B</td>
			<td>&nbsp;</td>
			<td><a href="https://knowles.com/docs/default-source/default-document-library/sph0645lm4h-1-datasheet.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>USB1</td>
			<td>1</td>
			<td>HOOYA</td>
			<td>USB-310F-B-SU</td>
			<td>&nbsp;</td>
			<td><a href="https://atta.szlcsc.com/upload/public/pdf/source/20181114/C309365_A6E5763ECAAC5EF4800A2E28BD762DDB.pdf">Datasheet</a></td>
			<td>&nbsp;</td>
		</tr>
	</tbody>
</table>
