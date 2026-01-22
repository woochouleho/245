<!DOCTYPE html>
<html>
<! Copyright (c) Deonet Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<script type="text/javascript" src="rollups/md5.js"></script>
<script type="text/javascript" src="php-crypt-md5.js"></script>
<SCRIPT language="javascript" src="/common.js"></SCRIPT>
<script type="text/javascript" src="/sha256_code.js"></script>
<meta http-equiv='Pragma' content='no-cache'>
<meta http-equiv="X-UA-Compatible" content="IE=edge"/>
<meta name="description" content=""/>
<meta name="keywords" content=""/>
<link rel="stylesheet" href="/admin/common.css"/>	

<SCRIPT>
#ifdef USE_CAPCTHA_OF_LOGINWEB
function createCaptcha() {
//  var code;
//  var canv = document.createElement("canvas");
//  canv.id = "captcha";
//  canv.width = 100;
//  canv.height = 30;
//  var ctx = canv.getContext("2d");
//  ctx.font = "bold italic 20px Batang";
  code = <% getCaptchastring(); %>;
//  ctx.fillText(code, 5, 25);
//  document.getElementById("captcha").appendChild(canv);
}
#endif

function isAttackXSS(str)
{
    i=0;
    for (i=0; i<str.length; i++) {
        if ((str.charAt(i) == '<')
                || (str.charAt(i) == '>')
                || (str.charAt(i) == '(')
                || (str.charAt(i) == ')')
                || (str.charAt(i) == '#')
                || (str.charAt(i) == '&')
                || (str.charAt(i) == '%')
                || (str.charAt(i) == '\'')
                || (str.charAt(i) == '\"')
                || (str.charAt(i) == '+')) {
            return true;
        }
    }
    return false;
}

function validateData(obj)
{
	var id = document.cmlogin.username.value;
	var pw = document.cmlogin.password.value;
	var captcha = document.cmlogin.captchaTextBox.value;

	if (isAttackXSS(id) || isAttackXSS(pw) || isAttackXSS(captcha)) {
		alert("ERROR: Some special characters are not supported.");
		return false;
	}

	setpass(obj);
}

function setpass(obj)
{
	document.cmlogin.encodePassword.value = deo_encrypt(document.cmlogin.password.value);
	document.cmlogin.password.disabled = true;
	<% passwd2xmit(); %>

	document.cmlogin.encodeUsername.value = deo_encrypt(document.cmlogin.username.value);
	document.cmlogin.username.disabled = true;

	obj.isclick = 1;
	postTableEncrypt(document.cmlogin.postSecurityFlag, document.cmlogin);
}

function mlhandle()
{
 postTableEncrypt(document.formML.postSecurityFlag, document.formML);
 document.formML.submit();
 parent.location.reload();
}
function refreshCaptcha(){
    location.reload();
}

function moveConnectionPage()
{
	<% CheckApConnection("check-ip"); %>
}

function initLoginInfo()
{
    var activeValue = "<% nvramGet("sw_active0"); %>";
    var versionValue;
	var webCookieElement = document.querySelector('input[name="web_cookie"]');

	webCookieElement.value = document.cookie;
    if (activeValue == "O") {
        versionValue = "<% nvramGet("sw_version0"); %>";
    }
    else {
        versionValue = "<% nvramGet("sw_version1"); %>";
    }

    var versionElement = document.getElementById("version");
    if (versionElement) {
        versionElement.innerText = "<% multilang(LANG_LOGIN_FIRM_VERSION); %>" 
                                + " : "
                                + versionValue;
    }
    var wan_nat_mode = "<% getInfo("wan_nat_mode"); %>";
    var modeElement = document.getElementById("mode");
    if (modeElement) {
        var wan_mode;
        if(wan_nat_mode == 1)
            wan_mode = "NAT";
        else
            wan_mode = "Bridge";
        modeElement.innerText = "<% multilang(LANG_LOGIN_MODE); %>" 
                                + " : " 
                                + wan_mode;
    }
	if (wan_nat_mode != 1){
		var optionWifiElement = document.getElementById("optionWifi");
		if (optionWifiElement) {
			optionWifiElement.style.display = "none";
		}
	}
}
</SCRIPT>
</head>
#ifdef USE_CAPCTHA_OF_LOGINWEB
<body class="new_login_bg" onload="createCaptcha()">
#else
<body class="new_login_bg">
#endif
<form action=/boaform/admin/userLogin method=POST name="cmlogin">
<input type="hidden" name="web_cookie" value="">
<input type="hidden" name="challenge">
<div id = "wrap">
<div class="new_login">
	<div class="logo">
		<img src="/graphics/sk_loing_logo.png" alt="" />
	</div>
	<div class="login_area">
		<ul>
		<li><input type="text" name="username" placeholder="<% multilang(LANG_ID); %>" /></li>
			<li><input type="password" name="password" placeholder="<% multilang(LANG_PASSWORD); %>" /></li>
		</ul>
#ifdef USE_CAPCTHA_OF_LOGINWEB
		<p class="txt_1"><% multilang(LANG_LOGIN_CAPCHA); %></p>

		<div class="automatic_typing_img">
			<div class="img_area">
				<!--이미지영역-->
				<ifname id="captcha" frameborder="0" marginwidth="0" marginheight="0" scrolling="no" width="100" height="35" style="position:absolute;left:45px;">
				<img src="/graphics/captcha.png" alt="" width="100" height="35">
			</ifname>

			</div>
			<button class="btn_refresh" type="button" onclick="refreshCaptcha()">
				<img src="/graphics/btn_refresh.gif" alt="<% multilang(LANG_LOGIN_REFRESH); %>">
			</button>
		</div>
		<div class="automatic_typing">
			<input type="text" name="captchaTextBox">
		</div>
#endif
		<div class="btn_login"> 
			<button class="login_btn"  type="submit" id="login_button" onClick="return validateData(this)">
				<span><% multilang(LANG_LOGIN_LOGIN); %></span>
			</button>
			<INPUT type=hidden name=encodeUsername value="">
			<INPUT type=hidden name=encodePassword value="">
			<INPUT type=hidden name=save value="1">

			<div id="optionWifi">
				<div style='margin-top:10px;'>
					<input style="height:50px" type="button" class="basic-btn00 btn-orange2-bg" id="apbtn" value="<% multilang(LANG_LOGIN_CONNECT_WIFI_MENU); %>" onClick="moveConnectionPage()" ></input>
				</div>
			</div>
			<div>
				<br>
				<center><% multilang(LANG_LOGIN_MODEL_NAME); %>: <p style="display:inline;">GNT245</p></center>
				<center><p id="version"><% multilang(LANG_LOGIN_FIRM_VERSION); %>: DGNT245SWXXXX </p></center>
				<center><p id="mode"><% multilang(LANG_LOGIN_MODE); %>: </p></center>
				<center><p id='reset_first'><% multilang(LANG_LOGIN_AUTORESET_TIME); %>:<% ShowAutoRebootTargetTime(); %></p></center>
			</div>
		</div>
	</div>
	<div class="internet_status">
		<ul class="clfx">
			<p></p>
		</ul>
	</div>
	<p class="copyright">&copy; DEONET, Inc. All rights reserved.</p>
	<br><br>
</div>

<input type="hidden" value="/" name="submit-url">
<input type="hidden" name="postSecurityFlag" value="">
</form>
<form action=/boaform/admin/userLoginMultilang method=POST name="formML">
<CENTER><TABLE cellSpacing=0 cellPadding=0 border=0>
<tr><td>
 <% checkWrite("loginSelinit"); %>
 <input type="hidden" name="postSecurityFlag" value="">
</td></tr>
</TABLE></CENTER>
</form>
<script>
initLoginInfo();
</script>
</body>
</html>
