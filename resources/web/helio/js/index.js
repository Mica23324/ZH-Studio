function IsInSlicer() {
    let bMatch = navigator.userAgent.match(RegExp('BBL-Slicer', 'i'));

    return bMatch;
}

function SendWXMessage(strMsg) {
    let bCheck = IsInSlicer();

    if (bCheck != null) {
        if (window.wx && typeof window.wx.postMessage === 'function') {
            window.wx.postMessage(strMsg);
        }
    }
}

function OpenPPLink() {
    SendWXMessage("helio_link_pp");
}

function OpenTouLink() {
    SendWXMessage("helio_link_tou");
}

function OpenHomeLink() {
    SendWXMessage("helio_link_home");
}

function getUrlParam(name) {
	const reg = new RegExp(`(^|&)${name}=([^&]*)(&|$)`);
	const r = window.location.search.substr(1).match(reg);
	return r ? decodeURIComponent(r[2]) : null;
}