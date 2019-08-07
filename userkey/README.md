# 创建密码重置盘的时候，系统都做了些什么

## 基本原理

密码重置盘的本质是一个RSA私钥，用于解密在系统中（为密码重置而专门）存储的用户密码。

创建密码重置盘时，系统会生成一个RSA公私钥对和一张自签名的证书。用户密码被公钥加密并签名，并和证书一起存储在计算机中（具体细节见下）；私钥则被保存在密码重置盘根目录下的`userkey.psw`文件中。

由这个过程，我们可以得到密码重置盘的几个性质：
- 密码重置盘与用户和密码密切相关；更换用户~~或用户更换密码~~（参见`README-More.md`），密码重置盘会失效。
- 系统识别密码重置盘只会看根目录下的`userkey.psw`文件，与其他文件一律无关。由此，使用一个可移动介质做多张密码重置盘是可行的，不过需要每次使用之前手动修改文件名。
- 密码重置盘只能重置密码是系统有意进行的限制。由密码重置盘恢复密码是可能的。

下文中，我的研究方向也就在“如何从密码重置盘恢复密码”上。对于其它方向涉及的可能不算深入。

## 技术细节

值得注意的是，微软在密码重置盘这个功能上基本没有做出过任何改动/改进。下文中，若非特殊说明，分析结论适用于自Windows XP开始所有版本的Windows。

### 0. 向导程序的手动调用方式

密码重置盘的创建向导和使用向导都在系统的凭据管理器`keymgr.dll`中，可以使用命令手动调用：

创建向导（当前用户）：`rundll32 keymgr.dll,PRShowSaveWizardEx`

创建向导（指定用户）：`rundll32 keymgr.dll,PRShowSaveFromMsginaW <用户名>`

重置向导（指定用户）：`rundll32 keymgr.dll,PRShowRestoreWizardEx <用户名>`（参见`README-More.md`）

在启动向导之前要确保电脑中已有可移动介质插入，否则向导会报错，拒绝启动。

### 1. 自签名证书

证书因为只做签名目的与恢复密码无关，不做深入研究。

生成的证书被放在本地计算机的一个名为`Recovery`的存储区中。这些证书具有如下性质：
- 颁发给/颁发者：空
- 有效期从：创建密码重置盘的时间
- 有效期至：创建密码重置盘的时间 + 5年
- 签名算法：SHA1
- 公钥：RSA 2048位

注意在没创建过密码重置盘的系统中，`Recovery`存储区是不存在的。

### 2. 密码重置盘/RSA私钥

上文所述的`userkey.psw`文件具有只读、归档属性。

在Windows XP上，`.psw`还有类型标注，名为`Password Backup`。同时在注册表中（`HKEY_CLASSES_ROOT\PSWFile`）还标注了`NoOpen`属性，即在试图打开psw类型文件时，会弹出一个“系统文件警告”提示。

文件的内容本质上是系统API `CryptExportKey`的导出结果，外加了一个8字节的文件头。私钥明文存储，文件结构见`userkey.h`。

由这个文件的内容，可以使用[这里提供的方法](https://stackoverflow.com/a/19855935)还原出一个标准格式的RSA私钥文件。具体操作不再赘述。

### 3. 加密的用户密码

这些数据（不出所料地）被放进了注册表。

在Windows XP上，具体的路径为`HKEY_LOCAL_MACHINE\SECURITY\Recovery\<SID>`的默认键值，类型为REG_BINARY，其中`<SID>`为用户的SID。

在Windows 7及以上版本，注册表路径变为了`HKEY_LOCAL_MACHINE\C80ED86A-0D28-40dc-B379-BB594E14EA1B\<SID>`（原文如此），而且这个项变成了按需加载；密码重置盘创建完毕时这个项就会被卸载。卸载后的注册表文件位于`%SystemRoot%\System32\Microsoft\Protect\Recovery\Recovery.dat`。

注意：无论是注册表还是卸载后的文件，都需要至少SYSTEM权限才能够读取。

这些REG_BINARY数据的结构如`regdata.h`所示。数据的最后256字节便是加密过的用户密码。解密步骤如下：
1. 取这256字节的数据，进行一次字节顺序翻转。
2. 设翻转后的数据保存为文件`enc.bin`，使用上面的步骤生成的DER格式私钥为`key.der`。使用OpenSSL解密数据：`openssl rsautl -decrypt -in enc.bin -out dec.bin -inkey key.der -keyform DER`。
3. 文件`dec.bin`中便是以Unicode格式（Windows口中的Unicode，即UTF-16 LE）存储的密码明文。

### X. 逆向工程的更多细节

向导的GUI界面，和密钥/证书生成部分的逻辑，都是在`keymgr.dll`中实现的。

由界面到生成逻辑的引用链如下所示（符号名称来自PDB）：
```
PRShowSaveWizardExW --> SPageProc1 --> SaveThread --> SaveInfo（进入逻辑部分）
    SaveInfo（返回时保存私钥） --> PRGenerateRecoveryKey（生成私钥） --> GenerateRecoveryCert（生成证书）
    PRGenerateRecoveryKey --> PRImportRecoveryKey（RPC，通知系统保存密码）

PRShowRestoreWizardExW --> PRShowRestoreWizardW --> RPageProc1 --> SetAccountPassword（进入逻辑部分）
    SetAccountPassword --> PRRecoverPassword（RPC，通知系统验证与修改密码）
```

两边的调用链都终止于RPC调用，确切的说是LPC端口`protected_storage`。有趣的是提供这个端口的服务`Protected Storage`服务早在Windows 8时就因为不安全而被移除了，但Windows 10中这个功能仍然可用。

查阅资料得到关键信息：这个端口下有`PasswordRecovery`接口，GUID为`5cbe92cb-f4be-45c9-9fc9-33e73e557b20`。使用RpcView工具轻易查到，提供这个接口的模块是由`lsass.exe`加载的`dpapisrv.dll`。

*P.S. RpcView的上次更新是2017年，又因为有对系统文件版本的检查，导致其无法使用。最终对程序进行了二进制修改。*

接口中的三个方法调用链如下：
```
keymgr!PRImportRecoveryKey --> s_SSRecoverImportRecoveryKey --> EncryptRecoveryPassword（构造注册表数据并保存） --> （各类细化的操作）

keymgr!PRRecoverPassword --> s_SSRecoverPassword --> ???（未进行分析）

??? --> s_SSRecoverQueryStatus --> ???（这个方法好像没用到）
```

注册表数据中的SHA1和签名分别由API `CertGetCertificateContextProperty`和内部函数`LogonCredGenerateSignature`生成，过程复杂，未进行分析。

负责加密的是API `BCryptEncrypt`函数，之后由`FMyReverseBytes`进行字节顺序翻转。

最后保存数据的是`RecoverySetSupplementalCredential`函数。由此可见，微软内部称密码重置盘的证书为“追加证书”。那个让前辈不明所以的GUID字符串`C80ED86A-0D28-40dc-B379-BB594E14EA1B`也得到了解释：这个字符串是硬编码进去的。

## 参考

- http://www.voidcn.com/article/p-morngjmj-mk.html
- https://docs.microsoft.com/en-us/windows/win32/seccrypto/rsa-schannel-key-blobs
- https://technet.microsoft.com/zh-cn/learning/aa380258(v=vs.100).aspx
- https://stackoverflow.com/a/19855935
- https://l.wzm.me/_security/internet/_internet/WinServices/ch04s10s15.html
- https://reverseengineering.stackexchange.com/a/8118
- https://github.com/silverf0x/RpcView

