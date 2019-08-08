## 0. 访问“本地计算机”证书存储区的方式

`certmgr.msc`所能够访问的证书存储区归属于当前登录的用户，而我们所要研究的那张自签名证书不在这些存储区里。

在Windows 7及更高版本的系统中，可以使用`certlm.msc`访问到“本地计算机”证书存储区。而在Windows XP中，步骤稍微复杂。

1. 启动管理控制台`mmc.exe`。
2. 选择“文件”->“添加/删除管理单元”菜单。
3. 在打开的窗口中点击“添加”按钮。此时会弹出一个列表。在列表中选择“证书”，点击“添加”。
4. 在“该管理单元将始终为下列账户管理证书”的选项中，选择“计算机账户”。
5. 一直“下一步”，返回到刚才的列表。关闭这个列表。
6. 点击“确定”。此时的控制台窗口中将会出现“证书(本地计算机)”一项。其使用方法与`certmgr.msc`相同。

## 1. 能否从系统中清除密码重置盘的痕迹？

只要用户制作过密码重置盘，在登录密码输入错误的时候，就会出现使用密码重置盘的提示。尤其是在Windows XP上，弹出的提示框特别显眼。那么，既然系统提供了创建密码重置盘的入口，那有没有移除密码重置盘的方式呢？

实践表明，只要删除了注册表数据和/或证书，登录密码输入错误的时候就不会再提示使用密码重置盘。可以推测，系统是通过查找用户SID对应的数据，再通过数据中的证书SHA1查找证书来判断其有没有可用的密码重置盘的。逆向分析证明了这一点，调用链如下：
```
msgina!HandleFailedLogon（登录界面，密码错误时触发） --> msgina!PRQueryStatus（RPC） --> dpapisrv!s_SSRecoverQueryStatus --> dpapisrv!SPRecoverQueryStatus --> dpapisrv!RecoverFindRecoveryPublic --> （各种操作）
```

最后的`RecoverFindRecoveryPublic`函数会先调用`RecoveryRetrieveSupplementalCredential`获取注册表数据，再根据其中存储的证书SHA1查找对应证书。和假说不同的是，找到证书之后又使用`LogonCredVerifySignature`对注册表数据进行了签名检查。并不是很清楚全零的签名（见下）为什么可以通过检查。

## 2. 密码重置盘与密码间的关系有多密切？

之前的研究结束后，翻阅[微软文档](https://support.microsoft.com/zh-cn/help/4490115/windows-change-or-reset-your-password)时惊奇地发现，*如果再次忘记了密码，可以使用同一密码重置盘。无需创建新的密码重置盘。*这使我怀疑系统保存用户密码的过程是否是必要的。

经过实验，使用密码重置盘后，注册表中保存的密码与签名也会同步变化。但是：
- 如果手动修改密码（没有修改注册表数据），事后密码重置盘仍然可用；
- 如果手动修改注册表数据（来源是之前的备份，没有修改密码），事后密码重置盘也仍然可用。

也就是说，系统保存的用户密码并不是修改密码的必要条件。为了获知系统存储用户密码的目的，接下来对上次没有逆向完的接口进行分析。

### 重置密码的过程

接上次对`dpapisrv.dll`中RPC接口的分析，重置密码的调用链如下：
```
keymgr!PRRecoverPassword --> s_SSRecoverPassword --> DecryptRecoveryPassword（获取、解密并判断注册表中的用户密码） --> （各类细化的操作）
    s_SSRecoverPassword（在DecryptRecoveryPassword返回后） --> ResetLocalUserPassword --> samsrv!SamIChangePasswordForeignUser（API，修改用户密码）
```

整条调用链中最神奇的就是`DecryptRecoveryPassword`函数。它拿着注册表数据，最终使用API `BCryptDecrypt`解密得到明文密码，然后……就没有然后了。判断能否进行密码重置的唯一条件是，提供的私钥能够解密注册表数据，而对解密得到的数据没有进行任何校验。而且好像这个函数对注册表数据中的签名也没有进行校验……不过这是后话，需要进一步分析。

明文密码之后被传入了`ResetLocalUserPassword`函数。然而，真正用来修改密码的API并没有使用这个参数。（和我的想象一致；系统自己修改自己的密码还需要校验，岂不是自己骗自己？）使用到它的只有来自于`lsasrv.dll`的一个接口的函数，跟踪接口编号发现是API `lsasrv!LsaINotifyPasswordChanged`，与修改密码无关。

至此，一个事实也被确定了下来：注册表中保存用户密码没有任何实际意义。非要说有的话，就是增加了一个信息泄露的攻击面吧……

### 签名的生成过程

接上次的分析，生成签名的部分调用链如下：
```
... --> EncryptRecoveryPassword --> LogonCredGenerateSignature --> LogonCredGenerateSignatureKey --> ...（没勇气看下去了）
```

这个先放放。真的太复杂了。大概瞟了几眼，发现签名中包含用户的二进制格式SID（但Windows XP的数据中好像没有），而且签名与用户密码有关（`FMyPrimitiveSHA`函数）。

*TODO*

## 3. 我有一个大胆的想法

一大顿分析下来，我得出的结论就是：微软对这个功能**真的不上心**！加密保存却没用到的密码、生成了却打酱油的签名……到处都是形同虚设的设计。那么，既然各种验证机制都没有起到应有的作用，密码重置盘这个机制能否成为新的攻击面？

### 能否伪造/无效化签名？

直接拿提取的注册表数据开刀，把签名部分整体抹零处理，再导入回注册表。结果不出所料：密码重置盘仍然正常工作。有趣的是，手动修改过签名之后再重置密码，注册表数据便不再与用户密码同步了，签名也维持着全零状态。

### 能否伪造证书/公私钥对？

本来两个测试应该分开的，但是懒得再手动提取私钥了……以后写个程序干了。

测试步骤有点多：
1. 使用OpenSSL生成一个2048位的私钥：`openssl genrsa -out priv.pem 2048`
2. 再把各个参数打印出来：`openssl rsa -in priv.pem -text`
3. 由这些参数捏出一个`userkey.psw`（注意字节序问题，和上述命令的输出有时有前导零的问题），备用。
4. 由刚才生成的私钥生成一张自签名证书：`openssl req -x509 -key priv.pem -out cert.pem`。最后还可以加一个`-days 365`参数指定证书有效时长。
5. 再把生成的证书转换为Windows下使用的格式：`openssl x509 -in cert.pem -outform DER -out cert.cer`
6. 打开这张证书，把它的SHA1值填入注册表数据。
7. 随便准备一点数据存入`dec.bin`，用来作为“用户的密码”。
8. 再用一次OpenSSL，加密数据：`openssl rsautl -encrypt -certin -inkey cert.pem -in dec.bin -out enc.bin`。把得到的数据填入注册表。
9. 至于签名，和上面一样置零就好。
10. 导入证书和注册表，安放好`userkey.psw`，见证奇迹。

嗯，见证奇迹失败，提示“重置磁盘无效”。错误码是`0x80090005 NTE_BAD_DATA`，代表着是解密时出了问题。又一次进入了漫长的逆向工程和登陆界面调试过程。

*P.S.：在不关闭Windows XP系统文件保护的前提下修改系统文件的方式：直接修改DllCache。（顿时又觉得微软的程序员们蠢萌蠢萌的）*

*P.S.2.：在Windows XP上，LPC端口`protected_storage`的另一端在`lsasrv.dll`，而不是`dpapisrv.dll`中，但接口的方法名没有变。*

### 小插曲：手动启动重置向导的正确方式

跑调试的过程中发现，使用rundll32手动启动的密码重置向导，无论是不是在登录画面启动，都会在重置密码的一步报错，进行不下去。调试发现错误码及其喜庆：`0x80008888`，继续深挖，是一个存储用户名的全局变量没有初始化，但是不知道原因。无奈，跑去分析`keymgr.dll`的入口点。

事先分析过进程，得知创建向导是用rundll32启动的没错。发现入口中使用了一个`GetNames`函数，而它的作用就是初始化各个全局变量，包括当前用户的用户名。回过头来看重置向导，在登录界面上由`msgina.dll`直接调用，用户名是使用参数传递进来的。加上用户名做rundll32的命令行参数（`PRShowRestoreFromMsginaW`和`PRShowRestoreWizardExW`是同一个函数），问题解决。

另外，在导出表中还发现了`PRShowSaveFromMsginaW`函数。能在登录界面上创建密码重置盘？

在`msgina.dll`中查找调用链外加实验，发现可能的这种启动创建向导的路径为：
- 用户登录时修改密码（“用户下次登录时需更改密码”标记）
- 用户登录后手动修改密码

算了，没用。不过这意味着这个入口可以直接调用，也就是为指定用户启动创建向导。这些发现已经写回到`README.md`中。

### 回到正题

在把自己的Windows XP虚拟机调试死机过N次后，我决定放弃，转而去翻文档。Windows XP下，执行RSA加密的函数是更加原始的`CryptEncrypt`（[文档见此](https://docs.microsoft.com/en-us/windows/win32/api/wincrypt/nf-wincrypt-cryptencrypt)）。文档一语点醒梦中人：*The ciphertext is returned in little-endian format.*

好的。才想起来存进注册表的数据忘记了字节颠倒。补上这一步操作再导入注册表，一次成功。

到现在为止，我们已经可以通过自行生成并导入一个用户的密码重置盘数据，修改任意账户的密码。所需要的仅仅是用户的SID（`wmic useraccount where name='%username%' get sid`），和……

### 蛋疼的权限

理想很丰满，但如果要从头伪造一份密码重置盘的数据，需要异常高的权限：
- 将证书导入“本地计算机”证书存储区：管理员权限
- 直接写入注册表数据：SYSTEM权限

另一个方案是去手动调用RPC接口，毕竟提供接口的服务是运行在`lsass.exe`进程中，具有SYSTEM权限。但负责导入数据的`s_SSRecoverImportRecoveryKey`函数，参数之一是用户当前的密码，而且会进行校验（`VerifyCredentials`函数）。

哎，又是一个伪漏洞。研究就先到这里了。

得知密码重置盘这个机制，已经是几年前的事了。从当时，我就觉得这个东西蛮有趣的，值得研究一下。了结了自己的一个心愿。

