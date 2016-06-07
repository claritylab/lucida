from threading import Lock


services_lock = Lock()

print_lock = Lock()

def log(s):
	print_lock.acquire()
	print(s)
	print_lock.release() 
