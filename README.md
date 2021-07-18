# Maho_Shojo
Maho_Shojo compiler 欢迎来到魔法少女的世界~

- [Maho_Shojo](#maho_shojo)
  - [References](#references)
  - [魔法少女调教日志](#魔法少女调教日志)
  - [魔法少女育成指南](#魔法少女育成指南)
  - [魔法少女使用指南](#魔法少女使用指南)
    - [flag说明:](#flag说明)

## References
竞赛所需资料
https://gitlab.eduxiji.net/nscscc/compiler2021/-/tree/master


## 魔法少女调教日志
> 06.06 完成词法、语法分析，语法树构建、TreePrinter,简单的语义检查checker。

> 07.18 完成MIR构建。准备进行目标平台代码生成。

## 魔法少女育成指南
```shell
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## 魔法少女使用指南
```shell
$ cd build
$ ./MHSJ [ -h | --help ] [ -p | --trace_parsing ] [ -s | --trace_scanning ] [ -emit-mir ] [ -emit-ast ] [-nocheck] [-o <output-file>] <input-file>
```

### flag说明:

`-h`或`--help`:帮助信息

`-p`或`--trace_parsing`追踪语法分析详情

`-s`或`--trace_scanning`追踪词法分析详情

`-emit-mir`生成MIR。未指定的情况下默认输出文件是`a.ll`

`-o <output-file>`指定MIR的输出文件。

`-emit-ast`通过ast复原代码，直接打印出来

`-nocheck`不进行静态检查

