var sRet = '';

function anchorToMarkdown(elem) {
	if(elem.childElementCount) {
		for(var i = 0; i < elem.children.length; i++)
			anchorToMarkdown(elem.children[i]);
	} else {
		if(elem.tagName !== 'A')
			return;
		sRet += `- [${elem.innerText}](${elem.href})\n`;
	}
}

anchorToMarkdown(document.body);

var ret = document.createElement('textarea');
ret.innerHTML = sRet;
document.body.appendChild(ret);

