#!/usr/bin/env python3

# This is the algorithm "Ruby"-based products used for generating ActiveX license keys
# Where "Ruby" being the codename for (classic) Visual Basic: https://retool.com/visual-basic

# The algorithm is implemented in a common function called CalcValue, and is called in following modules:
# 1. MSVBVM60!CPrivClassFactory::CreateInstanceLic, for licensing ActiveX controls
# 2. VB6!VBValidateLicense -> VB6!DoValidateLicense, for checking VB edition (Learning, Professional, Enterprise, etc.)
# 3. VBE7!VBEValidateLicense -> VBE7!DoValidateLicense, for checking whether Office 2000 Developer is installed (see below; the same applies to VBA6)

# For #1 it is pretty straightforward, just another algorithm for generating a license key from typelib IDs

# #2 checks several hardcoded "TypeLib ID" ("LICID") and sets VB6!Rby_Edition to a corresponding value (1 to 3, higher = more features)
# If no license is found, VB6!Rby_Edition is set to 0, corresponding to a "Working Model Edition" which cannot build executables

# #3 is basically the same as #2, but is a bit interesting since VBA don't have editions of sorts
# Only one hardcoded LICID is checked, and VBE7!Rby_Edition is set to 3 ("Enterprise") or 0 ("Working Model") based on its presence
# Which means VBA is running most of the time as a "Working Model", unless Office 2000 Developer is installed, which includes the license key (bundled with MSADDNDR.DLL, the "AddIn Designer"; see Setup\ode98ent.stf on CD)
# This license enables VBA's standalone mode, allowing it to create and open standalone VBA projects (with a .vba file extension) independent of the host application
# Moreover, a "Build DLL" option is enabled to compile standalone VBA projects into ActiveX DLL. It requires a LINK.EXE from VC6 era, which is provided by Office 2000 Developer
# Some of these standalone functionalities is briefly mentioned in VBA help files and VBA SDK documentations
# Unfortunely the build function is broken and unfixable since VBE7 and its 64-bit support; it still generates DLL files given a proper LINK.EXE, but the output is always 32-bit, linked against MSVBVM60.DLL, and most importantly, riddled with corruptions due to P-Code engine changes
# VBE interface code has also hardcoded all Class Modules to be private to the project (see resource TYPELIB#3 in VBE7.DLL; hardcoding happens in VBE7!ErrGetPropertyDispatch), which also prevents building a (ActiveX) DLL

# For reference, the generated license key LICKEY needs to be installed under this registry path to work:
# [HKEY_LOCAL_MACHINE\Software\CLASSES\Licenses\<LICID>]
# @="<LICKEY>"

# Previous work on the algorithm (which is super nice and detailed): https://blog.csdn.net/zzlufida/article/details/89574327
# Sadly VBGood.com is gone, and no proper attribution could thus be given to that guy who painfully compared the whole Windows registry to find out the VBA license key. Kudos to you, if you're reading this.
# The only remnant left on Internet I could find is this post in English: https://www.vbforums.com/showthread.php?891467

def ruby_license(licid: str) -> str:
    licid = licid.encode()
    chrbuf = [a ^ b for a, b in zip(licid, licid[ : : -1])]
    chrbuf.pop()
    chrbase = ord('a') + sum(chrbuf) % 10
    
    outbuf = bytearray()
    for x in chrbuf[ : (len(chrbuf) + 1) // 2]:
        outbuf.append(chrbase + (x & 0x0f))
        outbuf.append(chrbase + (x >> 4))
    return outbuf.decode()

licids = [
    '6000720D-F342-11D1-AF65-00A0C90DCA10', # VB6 Learning
    '74872840-703A-11d1-A3AF-00A0C90F26FA', # VB6 Professional
    '74872841-703A-11d1-A3AF-00A0C90F26FA', # VB6 Enterprise
    
    '8804558B-B773-11d1-BC3E-0000F87552E7', # VBE7 Developer
]
for licid in licids:
    print(licid, ruby_license(licid))

