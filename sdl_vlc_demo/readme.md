1. SDL_CreateWindowFrom resize window

 use SDL_CreateWindowFrom to create window.It can play video Normally. but if resize the window,the video will static.Why?how can i fix this bug?

2. sdl thread
  主线程中创建sdl窗口，在子线程中监听sdl事件，sdl窗口消息是阻塞的，子线程中无法监听事件
