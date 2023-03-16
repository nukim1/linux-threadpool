
# linux-threadpool

### Usage

```c
void func(void* arg)  {
	// do something
}

int main(void) {
	ThreadPool tpool = CreateThreadPool(16, 128);
	SyncObject sync_obj[128];
	for(int i = 0 ; i < 128; i++) 
		 sync_obj[i] = EnqueueTask(tpool, func, 0);
	for(int i = 0 ; i < 128; i++) 
		WaitForSyncObject(sync_obj[i]);
	DestroyThreadPool(tpool);
	
	return 0;
}
```
