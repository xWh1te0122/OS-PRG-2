# OS-PRG-2
<h3 style="color:gray;">程式碼部分以copilot為工具完成，透過一步一步了解架構去修正錯誤內容，並實作出同步機制。</h3>

### 編譯程式方法:

1. 使用Ubuntu(WSL) Terminal

2. 使用gcc編譯程式，指令如下:

```sh
gcc -o lab2-2 lab2-2.c
```

**檔案請照當下路徑做對應更改**

### 運行程式方法:

```sh
./lab2-2 <buffer_size> <producer_speed> <consumer_speed>
```

**例如:**

```sh
./lab2-2 10 1 2
```

**參數格式若輸入錯誤會報錯**
