# L3 内存虚拟化
* 虚拟化主要分为两个部分：
    * CPU 的虚拟化（进程）。
    * 内存的虚拟化。
## 地址空间
* 地址空间：对物理内存的抽象。

<center><img src=./picture/image1.png width=20% /></center>

* 一个进程的地址空间包含运行程序的所有内存状态
    * 程序的代码。
    * 栈（stack）：保存当前的函数调用信息，分配空间给局部变量，传递参数和返回值，在高地址空间，由高地址向低地址变化。
    * 堆（heap）：管理动态分配的用户内存，在低地址空间，由低地址向高地址变化。

## 地址转换

### 动态重定位
* 使用基址（base）寄存器和界限（bound）寄存器。
    * 基址寄存器：用来确定物理地址 `physical address = virtual address + base`
    * 界限寄存器：提供访问保护，进程访问超过这个界限或者为负数的虚拟地址时 CPU 会触发异常。
    <center><img src=./picture/image2.png width=20% /></center>

> 我们将 CPU 的这个负责地址转换的部分统称为内存管理单元（Memory Management Unit，MMU）。

* 问题：效率较低，如下图所示，重定位的进程使用了从 32KB 到 48KB 的物理内存，但由于该进程的栈区和堆区并不很大，导致了大量的内部碎片。
> 内部碎片：在已经分配的内存单元内部有未使用的空间。

<center><img src=./picture/image3.png width=20% /></center>

### 分段
#### 分段的基本概念
* 在 MMU 中给每个地址空间内的逻辑段一对基址和界限寄存器对，一个段只对应地址空间里的一个连续定长的区域，典型的地址空间可分为代码、栈和堆段。
* 分段机制使得操作系统能够将不同的段放到不同的物理内存区域，从而避免了虚拟地址空间中栈与堆之间的内部碎片问题。

<center><img src=./picture/image4.png width=20% /></center>
<center><img src=./picture/image5.png width=20% /></center>
* 上图是一个例子
    * 如表中所示，代码段放在物理地址 32KB，大小为 2KB；堆在 34KB，大小也为 2KB。
    * 假设要引用代码段中的虚拟地址 100，由于 100 比 2KB 小，在界限内。因此有 `物理地址 = 100 + 32KB = 32868`。
    * 假设要引入的堆中的虚拟地址为 4200，则堆的实际偏移量为 `4200 - 4096 = 104`，，由于 104 比 2KB 小。因此有 `物理地址 = 104 + 34KB = 34920`。
    * 如果是要访问栈的话，根据增长标记为（Grows Positive），物理地址应该是减，而不是加。
    * 如果试图访问非法的地址则会产生段错误（segmentation fault）。

* 在显式方式中，使用虚拟地址的开始几位标记不同的段，用后面的位标记段内偏移量。
<center><img src=./picture/image6.png width=20% /></center>

* 访问方式写成代码如下所示：
    ```c
    // 得到虚拟地址的高 2 bit
    Segment = (VirtualAddress & SEG_MASK) >> SEG_SHIFT
    // 得到段内偏移量
    Offset = VirtualAddress & OFFSET_MASK
    if (Offset >= Bounds[Segment])
        RaiseException(PROTECTION_FAULT)
    else
        PhysAddr = Base[Segment] + Offset
        Register = AccessMemory(PhysAddr)
    ```

* 问题：可能会导致外部碎片，如下图所示，当需要分配一个 20KB 的段时，当前 24KB 空闲的空间不连续，导致操作系统无法满足这个 20KB 的请求。
<center><img src=./picture/image7.png width=20% /></center>

#### 空闲空间管理
