# Changelog

### 1.0.0 May 2024

- CH_FIXED_SIZE channels : ch_put will block when channel is full to avoid finishing memory is consumers are slower 
than producers

### 0.9.0 Feb 2024

- fixes to libchannel : ch_get() in blocking mode should loop in cond_wait()
