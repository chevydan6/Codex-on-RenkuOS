import subprocess,os,tempfile
r=tempfile.mkdtemp(prefix='codex-daemon-cycle-',dir='/CodexBuild/tmp');e=os.environ.copy();e['CODEX_HOME']=r
b='/CodexBuild/renkuos-codex-google-login/renkuos-native-daemon/bin/codex'
try:
 for action in ('start','start','restart','version','stop'):
  p=subprocess.run([b,'app-server','daemon',action],stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=e,timeout=45)
  print(action,'exit',p.returncode,p.stdout.decode('utf8','replace')[:900],flush=True)
  assert p.returncode==0,(action,p.returncode)
finally:
 subprocess.run([b,'app-server','daemon','stop'],env=e,stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=30)
print('daemon lifecycle passed',r,flush=True)
