import subprocess
import time

time.sleep(5)
hacker = r"C:\Program Files (x86)\Resource Hacker\ResourceHacker.exe"
cmd = rf'"{hacker}" -open "bin/neutralino-win_x64.exe" -save "bin/neutralino-win_x64.exe" -action addoverwrite -res "ico.ico" -mask ICONGROUP,MAINICON,0'
subprocess.call(cmd, shell=True)
