import subprocess,tempfile,os,time,json,signal
b=os.environ.get('CODEX_TEST_BINARY','/CodexBuild/renkuos-codex-google-login/renkuos-native-daemon/bin/codex')
r=tempfile.mkdtemp(prefix='codex-protocol-',dir='/CodexBuild/tmp');e=os.environ.copy();e.update(CODEX_HOME=r,HOME=r)
for label,data in [('invalid-json',b'{broken\n'),('unknown-method',b'{"id":1,"method":"does/not/exist","params":{}}\n'),('empty-eof',b'')]:
 p=subprocess.Popen([b,'app-server','--stdio'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=e,cwd=r)
 try:
  out,err=p.communicate(data,timeout=90)
  print(label,'exit',p.returncode,'panic',b'panicked' in err,'output',out[:600].decode('utf8','replace'),flush=True)
 except subprocess.TimeoutExpired:
  p.kill();out,err=p.communicate();print(label,'timeout killed','stderr',err[-400:].decode('utf8','replace'),flush=True)
p=subprocess.Popen([b,'app-server','--stdio'],stdin=subprocess.PIPE,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,env=e,cwd=r)
time.sleep(3);p.kill();p.wait();print('forced-stop',p.returncode,flush=True)
p=subprocess.Popen([b,'app-server','--stdio'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=e,cwd=r)
try:
 out,err=p.communicate(b'',timeout=90);print('restart-after-kill',p.returncode,'panic',b'panicked' in err,flush=True)
except subprocess.TimeoutExpired:
 p.kill();p.communicate();print('restart timeout',flush=True)
print('test home',r,flush=True)
