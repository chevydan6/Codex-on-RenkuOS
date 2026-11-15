import subprocess,re,time,os,signal

def teams():
 out=subprocess.check_output(['ps']).decode('utf8','replace');result={}
 for line in out.splitlines():
  m=re.match(r'^(.*?)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s*$',line)
  if m:result[int(m[2])]=m[1]
 return result
before=teams();created={}
try:
 for n in range(3):
  p=subprocess.run(['/boot/home/Desktop/Codex'],timeout=10)
  assert p.returncode==0,p.returncode
 time.sleep(3)
 after=teams();created={p:a for p,a in after.items() if p not in before and a.startswith('/boot/system/apps/Terminal -t Codex on RenkuOS')}
 print('new Terminal teams',created,flush=True);assert len(created)==3,created
 out=subprocess.check_output(['top','-d','-i','1','-n','1']).decode('utf8','replace')
 for line in out.splitlines():
  parts=line.split()
  if parts and parts[0].isdigit() and int(parts[0]) in created:
   cpu=float(parts[4]);print('launcher terminal CPU',parts[0],cpu,flush=True);assert cpu<5,(line,cpu)
 print('three launches returned, no busy Terminal main thread observed',flush=True)
finally:
 for p in created:
  try:os.kill(p,signal.SIGTERM)
  except ProcessLookupError:pass
