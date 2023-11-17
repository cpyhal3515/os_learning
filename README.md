# 操作系统学习记录


## 说明
> 目前作为学习 JYY 老师课程的练习，之前看过一遍王道考研课程中的操作系统课程，但是感觉大部分只是了解概念，对于实际的原理不是很清楚，所以就又找来 JYY 老师的课程学习，尽量完成课程中的基础作业。


* 这里是 B 站的课程视频：[南京大学2022操作系统-蒋炎岩](https://www.bilibili.com/video/BV1Cm4y1d7Ur/?spm_id_from=333.788&vd_source=60ce9938ee3696b9c3fa6dc847e7d86e) 
* 这里是课程主页：[操作系统设计与实现 (2022 春季学期)](http://jyywiki.cn/OS/2022/)

![网站课程内容](./picture/%E7%BD%91%E7%AB%99%E5%86%85%E5%AE%B9.png )


## 学习记录

* 2023年11月17日：看完了 1 和 2 部分，现在开始做一下 M1 的 pstree 作业, 
M1: 打印进程树 (pstree)。


## 工具说明
### 1、tmux 工具
* tmux 是基于 Unix 操作系统的终端多路复用器。它允许用户在同一终端内创建多个窗格，这对想要运行单独的进程或命令并同时预览输出的用户非常有用,
[tmux 使用方法](https://blog.gtwang.org/linux/linux-tmux-terminal-multiplexer-tutorial/)。

* 需要注意的是快捷键需要先按 ctrl+b 激活，之后加的比如 % 这种符号在键盘数字的上面就需要 shift + % 才可以。

× 最基本的三个用法：
* `Ctrl+b 再输入 %`：垂直分割终端
* `Ctrl+b 再输入 "` ：水平分割终端
* `Ctrl+b 再输入 方向鍵` ：切换至指定方向的 pane
