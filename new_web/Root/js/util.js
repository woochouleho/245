function isHexaDigit(digit) {
   var hexVals = new Array("0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                           "A", "B", "C", "D", "E", "F", "a", "b", "c", "d", "e", "f");
   var len = hexVals.length;
   var i = 0;
   var ret = false;

   for ( i = 0; i < len; i++ )
      if ( digit == hexVals[i] ) break;

   if ( i < len )
      ret = true;

   return ret;
}

function isDecDigit(digit) {
   var DecVals = new Array("0", "1", "2", "3", "4", "5", "6", "7", "8", "9");
   var len = DecVals.length;
   var i = 0;
   var ret = false;

   for ( i = 0; i < len; i++ )
      if ( digit == DecVals[i] ) break;

   if ( i < len )
      ret = true;

   return ret;
}

function isValidDecNum(val) {
   for (var i = 0; i < val.length; i++ )
      if ( isDecDigit(val.charAt(i)) == false )
         return false;

   return true;
}

function is_present_special_char(val)
{
	for (var i=0; i<val.length; i++) {
		if ((val.charAt(i) >= ' ' && val.charAt(i) <= '/')||	// space, !"#$%&`()*+'-./
				(val.charAt(i) >= ':' && val.charAt(i) <= '@') ||	// :;<=>?@
				(val.charAt(i) >= '[' && val.charAt(i) <= '`') ||	// [\]^_`
				(val.charAt(i) >= '{' && val.charAt(i) <= '~'))	// {|}~
		{
			return true;
		}
	}
	return false;
}

function is_present_number(val)
{
	for (var i=0; i<val.length; i++) {
		if ((val.charAt(i) >= '0') && (val.charAt(i) <= '9'))
			return true;
	}
	return false;
}

function is_present_string(val)
{
	for (var i=0; i<val.length; i++) {
		if ((val.charAt(i) >= 'A' && val.charAt(i) <= 'Z')||
				(val.charAt(i) >= 'a' && val.charAt(i) <= 'z'))
			return true;
	}
	return false;
}

function is_present_rc2(str)
{
	var rc = 0;

	if (is_present_string(str)) rc++;
	if (is_present_number(str)) rc++;
	if (is_present_special_char(str)) rc++;

	if (rc >= 2) return 1;
	return 0;
}

function isValidKey(val, size) {
   var ret = false;
   var len = val.length;
   var dbSize = size * 2;

   if ( len == size )
      ret = true;
   else if ( len == dbSize ) {
      for ( i = 0; i < dbSize; i++ )
         if ( isHexaDigit(val.charAt(i)) == false )
            break;
      if ( i == dbSize )
         ret = true;
   } else
      ret = false;

   return ret;
}


function isValidHexKey(val, size) {
   var ret = false;
   if (val.length == size) {
      for ( i = 0; i < val.length; i++ ) {
         if ( isHexaDigit(val.charAt(i)) == false ) {
            break;
         }
      }
      if ( i == val.length ) {
         ret = true;
      }
   }

   return ret;
}


function isNameUnsafe(compareChar) {
   var unsafeString = "\"<>%\\^[]`\+\$\,='#&@.: \t";

   if ( unsafeString.indexOf(compareChar) == -1 && compareChar.charCodeAt(0) > 32
        && compareChar.charCodeAt(0) < 123 )
      return false; // found no unsafe chars, return false
   else
      return true;
}

// Check if a name valid
function isValidName(name) {
   var i = 0;

   for ( i = 0; i < name.length; i++ ) {
      if ( isNameUnsafe(name.charAt(i)) == true )
         return false;
   }

   return true;
}

// same as is isNameUnsafe but allow spaces
function isCharUnsafe(compareChar) {
   var unsafeString = "\"<>%\\^[]`\+\$\,='#&@.:\t";

   if ( unsafeString.indexOf(compareChar) == -1 && compareChar.charCodeAt(0) >= 32
        && compareChar.charCodeAt(0) < 123 )
      return false; // found no unsafe chars, return false
   else
      return true;
}

function isValidNameWSpace(name) {
   var i = 0;

   for ( i = 0; i < name.length; i++ ) {
      if ( isCharUnsafe(name.charAt(i)) == true )
         return false;
   }

   return true;
}

function isSameSubNet(lan1Ip, lan1Mask, lan2Ip, lan2Mask) {

   var count = 0;

   lan1a = lan1Ip.split('.');
   lan1m = lan1Mask.split('.');
   lan2a = lan2Ip.split('.');
   lan2m = lan2Mask.split('.');

   for (i = 0; i < 4; i++) {
      l1a_n = parseInt(lan1a[i]);
      l1m_n = parseInt(lan1m[i]);
      l2a_n = parseInt(lan2a[i]);
      l2m_n = parseInt(lan2m[i]);
      if ((l1a_n & l1m_n) == (l2a_n & l2m_n))
         count++;
   }
   if (count == 4)
      return true;
   else
      return false;
}


function isValidIpAddress(address) {

   ipParts = address.split('/');
   if (ipParts.length > 2) return false;
   if (ipParts.length == 2) {
      num = parseInt(ipParts[1]);
      if (num <= 0 || num > 32)
         return false;
   }

   if (ipParts[0] == '0.0.0.0' ||
       ipParts[0] == '255.255.255.255' )
      return false;

   addrParts = ipParts[0].split('.');
   if ( addrParts.length != 4 ) return false;
   for (i = 0; i < 4; i++) {
      if (isNaN(addrParts[i]) || addrParts[i] =="")
         return false;
  num = parseInt(addrParts[i]);
  if ( num < 0 || num > 255 )
         return false;
   }
   return true;
}

function substr_count (haystack, needle, offset, length)
{
    var pos = 0, cnt = 0;

    haystack += '';
    needle += '';
    if (isNaN(offset)) {offset = 0;}
    if (isNaN(length)) {length = 0;}
    offset--;

    while ((offset = haystack.indexOf(needle, offset+1)) != -1){
        if (length > 0 && (offset+needle.length) > length){
            return false;
        } else{
            cnt++;
        }
    }

    return cnt;
}

function test_ipv6(ip)
{
  // Test for empty address
  if (ip.length<3)
  {
	return ip == "::";
  }

  // Check if part is in IPv4 format
  if (ip.indexOf('.')>0)
  {
        lastcolon = ip.lastIndexOf(':');

        if (!(lastcolon && isValidIpAddress(ip.substr(lastcolon + 1))))
            return false;

        // replace IPv4 part with dummy
        ip = ip.substr(0, lastcolon) + ':0:0';
  }

  // Check uncompressed
  if (ip.indexOf('::')<0)
  {
    var match = ip.match(/^(?:[a-f0-9]{1,4}:){7}[a-f0-9]{1,4}$/i);
    return match != null;
  }

  // Check colon-count for compressed format
  if (substr_count(ip, ':'))
  {
    var match = ip.match(/^(?::|(?:[a-f0-9]{1,4}:)+):(?:(?:[a-f0-9]{1,4}:)*[a-f0-9]{1,4})?$/i);
    return match != null;
  }

  // Not a valid IPv6 address
  return false;
}

function isValidIpAddress6(address) {
   ipParts = address.split('/');
   if (ipParts.length > 2) return false;
   if (ipParts.length == 2) {
      num = parseInt(ipParts[1]);
      if (num <= 0 || num > 128)
         return false;
   }

   return test_ipv6(ipParts[0]);
}

function isValidPrefixLength(prefixLen) {
   var num;

   num = parseInt(prefixLen);
   if (num <= 0 || num > 128)
      return false;
   return true;
}

function areSamePrefix(addr1, addr2) {
   var i, j;
   var a = [0, 0, 0, 0, 0, 0, 0, 0];
   var b = [0, 0, 0, 0, 0, 0, 0, 0];

   addr1Parts = addr1.split(':');
   if (addr1Parts.length < 3 || addr1Parts.length > 8)
      return false;
   addr2Parts = addr2.split(':');
   if (addr2Parts.length < 3 || addr2Parts.length > 8)
      return false;
   j = 0;
   for (i = 0; i < addr1Parts.length; i++) {
      if ( addr1Parts[i] == "" ) {
		 if ((i != 0) && (i+1 != addr1Parts.length)) {
			j = j + (8 - addr1Parts.length + 1);
		 }
		 else {
		    j++;
		 }
	  }
	  else {
         a[j] = parseInt(addr1Parts[i], 16);
		 j++;
	  }
   }
   j = 0;
   for (i = 0; i < addr2Parts.length; i++) {
      if ( addr2Parts[i] == "" ) {
		 if ((i != 0) && (i+1 != addr2Parts.length)) {
			j = j + (8 - addr2Parts.length + 1);
		 }
		 else {
		    j++;
		 }
	  }
	  else {
         b[j] = parseInt(addr2Parts[i], 16);
		 j++;
	  }
   }
   //only compare 64 bit prefix
   for (i = 0; i < 4; i++) {
      if (a[i] != b[i]) {
	     return false;
	  }
   }
   return true;
}

function getLeftMostZeroBitPos(num) {
   var i = 0;
   var numArr = [128, 64, 32, 16, 8, 4, 2, 1];

   for ( i = 0; i < numArr.length; i++ )
      if ( (num & numArr[i]) == 0 )
         return i;

   return numArr.length;
}

function getRightMostOneBitPos(num) {
   var i = 0;
   var numArr = [1, 2, 4, 8, 16, 32, 64, 128];

   for ( i = 0; i < numArr.length; i++ )
      if ( ((num & numArr[i]) >> i) == 1 )
         return (numArr.length - i - 1);

   return -1;
}

function isValidSubnetMask(mask) {
   var i = 0, num = 0;
   var zeroBitPos = 0, oneBitPos = 0;
   var zeroBitExisted = false;

   if ( mask == '0.0.0.0' )
      return false;

   maskParts = mask.split('.');
   if ( maskParts.length != 4 ) return false;

   for (i = 0; i < 4; i++) {
      if ( isNaN(maskParts[i]) == true )
         return false;
      num = parseInt(maskParts[i]);
      if ( num < 0 || num > 255 )
         return false;
      if ( zeroBitExisted == true && num != 0 )
         return false;
      zeroBitPos = getLeftMostZeroBitPos(num);
      oneBitPos = getRightMostOneBitPos(num);
      if ( zeroBitPos < oneBitPos )
         return false;
      if ( zeroBitPos < 8 )
         zeroBitExisted = true;
   }

   return true;
}

function isValidPort(port) {
   var fromport = 0;
   var toport = 100;

   portrange = port.split(':');
   if ( portrange.length < 1 || portrange.length > 2 ) {
       return false;
   }
   if ( isNaN(portrange[0]) )
       return false;
   fromport = parseInt(portrange[0]);

   if ( portrange.length > 1 ) {
       if ( isNaN(portrange[1]) )
          return false;
       toport = parseInt(portrange[1]);
       if ( toport <= fromport )
           return false;
   }

   if ( fromport < 1 || fromport > 65535 || toport < 1 || toport > 65535 )
       return false;

   return true;
}

function isValidNatPort(port) {
   var fromport = 0;
   var toport = 100;

   portrange = port.split('-');
   if ( portrange.length < 1 || portrange.length > 2 ) {
       return false;
   }
   if ( isNaN(portrange[0]) )
       return false;
   fromport = parseInt(portrange[0]);

   if ( portrange.length > 1 ) {
       if ( isNaN(portrange[1]) )
          return false;
       toport = parseInt(portrange[1]);
       if ( toport <= fromport )
           return false;
   }

   if ( fromport < 1 || fromport > 65535 || toport < 1 || toport > 65535 )
       return false;

   return true;
}

function isValidMacAddress(address) {
	var c = '';
	var num = 0;
	var i = 0, j = 0;
	var zeros = 0;

	/* Neon-14, refs #234 : do not allow more than basic MAC address length */
	/* MAC Address MUST be 00:00:00:00:00:00 format */
	if(address.length > 17) return false;

	addrParts = address.split(':');
	if ( addrParts.length != 6 ) return false;

	for (i = 0; i < 6; i++) {
		if ( addrParts[i] == '' )
			return false;
		for ( j = 0; j < addrParts[i].length; j++ ) {
			/* Neon-14, refs #234 : do not allow more than 2 digit */
			if(addrParts[i].length > 2) return false;

			c = addrParts[i].toLowerCase().charAt(j);
			if( (c >= '0' && c <= '9') ||
				(c >= 'a' && c <= 'f') )
				continue;
			else
				return false;
		}

		num = parseInt(addrParts[i], 16);
		if ( num == NaN || num < 0 || num > 255 )
			return false;
		if ( num == 0 )
			zeros++;
	}
	if (zeros == 6)
		return false;

	return true;
}

function isValidMacMask(mask) {
   var c = '';
   var num = 0;
   var i = 0, j = 0;
   var zeros = 0;
   var zeroBitPos = 0, oneBitPos = 0;
   var zeroBitExisted = false;

   maskParts = mask.split(':');
   if ( maskParts.length != 6 ) return false;

   for (i = 0; i < 6; i++) {
      if ( maskParts[i] == '' )
         return false;
      for ( j = 0; j < maskParts[i].length; j++ ) {
         c = maskParts[i].toLowerCase().charAt(j);
         if ( (c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') )
            continue;
         else
            return false;
      }

      num = parseInt(maskParts[i], 16);
      if ( num == NaN || num < 0 || num > 255 )
         return false;
      if ( zeroBitExisted == true && num != 0 )
         return false;
      if ( num == 0 )
         zeros++;
      zeroBitPos = getLeftMostZeroBitPos(num);
      oneBitPos = getRightMostOneBitPos(num);
      if ( zeroBitPos < oneBitPos )
         return false;
      if ( zeroBitPos < 8 )
         zeroBitExisted = true;
   }
   if (zeros == 6)
      return false;

   return true;
}

var hexVals = new Array("0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
              "A", "B", "C", "D", "E", "F");
var unsafeString = "\"<>%\\^[]`\+\$\,'#&";
// deleted these chars from the include list ";", "/", "?", ":", "@", "=", "&" and #
// so that we could analyze actual URLs

function isUnsafe(compareChar)
// this function checks to see if a char is URL unsafe.
// Returns bool result. True = unsafe, False = safe
{
   if ( unsafeString.indexOf(compareChar) == -1 && compareChar.charCodeAt(0) > 32
        && compareChar.charCodeAt(0) < 123 )
      return false; // found no unsafe chars, return false
   else
      return true;
}

function decToHex(num, radix)
// part of the hex-ifying functionality
{
   var hexString = "";
   while ( num >= radix ) {
      temp = num % radix;
      num = Math.floor(num / radix);
      hexString += hexVals[temp];
   }
   hexString += hexVals[num];
   return reversal(hexString);
}

function reversal(s)
// part of the hex-ifying functionality
{
   var len = s.length;
   var trans = "";
   for (i = 0; i < len; i++)
      trans = trans + s.substring(len-i-1, len-i);
   s = trans;
   return s;
}

function convert(val)
// this converts a given char to url hex form
{
   return  "%" + decToHex(val.charCodeAt(0), 16);
}


function hexToAscii(hexstr)
{
   var hex = hexstr.toString();
   var str = '';
   for ( var i = 0 ; i < hexstr.length; i += 2 )
     str += String.fromCharCode(parseInt(hex.substr(i,2), 16));
   return str;
}

function asciiToHex(asciiStr)
{
   var hexStr = '';
   for ( var i = 0 ; i < asciiStr.length ; i ++ )
      hexStr += Number(asciiStr.charCodeAt(i)).toString(16);

   return hexStr;
}

function transKey(key)
{
   if (key.length == 13 || key.length == 5) // ascii str
      return asciiToHex(key);
   return key;
}

function isAsciiStr(str)
{
   for ( var i = 0 ; i < str.length; i += 2 )
   {
     code = parseInt(str.substr(i,2), 16);
     if (code < 32 || code > 123)
        return false;
   }
   return true;
}

function transKeyView(key)
{
   if (isAsciiStr(key))
      return hexToAscii(key);
   return key;
}

function encodeUrl(val)
{
   var len     = val.length;
   var i       = 0;
   var newStr  = "";
   var original = val;

   for ( i = 0; i < len; i++ ) {
      if ( val.substring(i,i+1).charCodeAt(0) < 255 ) {
         // hack to eliminate the rest of unicode from this
         if (isUnsafe(val.substring(i,i+1)) == false)
            newStr = newStr + val.substring(i,i+1);
         else
            newStr = newStr + convert(val.substring(i,i+1));
      } else {
         // woopsie! restore.
         alert ("Found a non-ISO-8859-1 character at position: " + (i+1) + ",\nPlease eliminate before continuing.");
         newStr = original;
         // short-circuit the loop and exit
         i = len;
      }
   }

   return newStr;
}

var markStrChars = "\"'";

// Checks to see if a char is used to mark begining and ending of string.
// Returns bool result. True = special, False = not special
function isMarkStrChar(compareChar)
{
   if ( markStrChars.indexOf(compareChar) == -1 )
      return false; // found no marked string chars, return false
   else
      return true;
}

// use backslash in front one of the escape codes to process
// marked string characters.
// Returns new process string
function processMarkStrChars(str) {
   var i = 0;
   var retStr = '';

   for ( i = 0; i < str.length; i++ ) {
      if ( isMarkStrChar(str.charAt(i)) == true )
         retStr += '\\';
      retStr += str.charAt(i);
   }

   return retStr;
}

// Web page manipulation functions

function showhide(element, sh)
{
    var status;
    if (sh == 1) {
        status = "block";
    }
    else {
        status = "none"
    }

	if (document.getElementById)
	{
		// standard
		document.getElementById(element).style.display = status;
	}
	else if (document.all)
	{
		// old IE
		document.all[element].style.display = status;
	}
	else if (document.layers)
	{
		// Netscape 4
		document.layers[element].display = status;
	}
}

// Load / submit functions

function getSelect(item)
{
	var idx;
	if (item.options.length > 0) {
	    idx = item.selectedIndex;
	    return item.options[idx].value;
	}
	else {
		return '';
    }
}

function setSelect(item, value)
{
	for (i=0; i<item.options.length; i++) {
        if (item.options[i].value == value) {
        	item.selectedIndex = i;
        	break;
        }
    }
}

function setCheck(item, value)
{
    if ( value == '1' ) {
         item.checked = true;
    } else {
         item.checked = false;
    }
}

function setInnerHtml(item,value) {
	document.getElementById(item).innerHTML=value;
}

function setDisable(item, value)
{
    if ( value == 1 || value == '1' ) {
         item.disabled = true;
    } else {
         item.disabled = false;
    }
}

function submitText(item)
{
	return '&' + item.name + '=' + item.value;
}

function submitSelect(item)
{
	return '&' + item.name + '=' + getSelect(item);
}


function submitCheck(item)
{
	var val;
	if (item.checked == true) {
		val = 1;
	}
	else {
		val = 0;
	}
	return '&' + item.name + '=' + val;
}

function getURLVars(url) {
	var vars = {};
	var parts = url.replace(/[?&]+([^=&]+)=([^&]*)/gi,
			function(m,key,value) {
				vars[key] = value;
			});
	return vars;
}

function showmenubar() {
document.write('<div class="header_bg">'+
	'<div class="top_wrap">' +
		'<div class="top_area clfx">' +
			'<div class="logo">' +
				'<a href="basic_status.html">' +
					'<img src="img/new/logo.png" alt="SK broadband." /> <span>H854AX</span>' +
				'</a>' +
			'</div>' +
			'<div class="util web clfx">' +
				'<span>root</span>' +
				'<button class="btn_logout" onclick="window.location.href=\'logout.html\'">로그아웃</button>' +
				'<ul class="btn_setting clfx">' +
					'<li class="on"><button>기본설정</button></li>' +
					'<li><button>고급설정</button></li>' +
				'</ul>' +
			'</div>' +
			'<div class="gnb_open"><a href="#"><img src="img/gnb_menu.png" alt="" /></a></div>' +
		'</div>' +
	'</div>' +
	'<div class="header">' +
		'<div class="gnb">' +
			'<div class="gnb_close"><a href="#"><img src="img/gnb_close.png" alt="" /></a></div>' +
			'<div class="util mb">' +
				'<span>root</span>' +
				'<button class="btn_logout" onclick="window.location.href=\'logout.html\'">로그아웃</button>' +
				'<ul class="btn_setting clfx">'  +
					'<li class="on"><button>기본설정</button></li>'  +
					'<li><button>고급설정</button></li>' +
				'</ul>' +
			'</div>' +
			'<ul class="clfx basic_gnb">' +
				'<li class="gnb_1 mb_gnb">' +
					'<a href="basic_status.html">기본설정</a>' +
					'<div class="s_gnb">' +
						'<ul>' +
							'<li><a href="basic_status.html">시스템 상태 정보</a></li>' +
							'<li><a href="basic_wan.html">인터넷 설정</a></li>' +
							'<li><a href="basic_wlan.html">무선 설정</a></li>' +
							'<li><a href="basic_ip_list.html">접속 단말 정보</a></li>' +
						'</ul>' +
					'</div>' +
				'</li>' +
			'</ul>' +
			'<ul class="clfx advanced_gnb">' +
				'<li class="gnb_1 mb_gnb">' +
					'<a href="basic_status.html">기본설정</a>' +
					'<div class="s_gnb">' +
						'<ul>' +
							'<li><a href="basic_status.html">시스템 상태 정보</a></li>' +
							'<li><a href="basic_wan.html">인터넷 설정</a></li>' +
							'<li><a href="basic_wlan.html">무선 설정</a></li>' +
							'<li><a href="basic_ip_list.html">접속 단말 정보</a></li>' +
						'</ul>' +
					'</div>' +
				'</li>' +
				'<li class="gnb_2 mb_gnb">' +
					'<a href="network_status.html">네트워크</a>' +
					'<div class="s_gnb">' +
						'<ul>' +
							'<li><a href="network_status.html">네트워크 상태 정보</a></li>' +
							'<li><a href="network_setup.html">네트워크 설정</a></li>' +
							'<li><a href="network_port.html">유선포트 설정</a></li>' +
							'<li><a href="network_ipv6.html">IPv6 설정</a></li>' +
							'<li><a href="network_iptv.html">IPTV</a></li>' +
							'<li><a href="network_vlan.html">VLAN</a></li>' +
							'<li><a href="network_qos.html">유선 QoS 설정</a></li>' +
						'</ul>' +
					'</div>' +
				'</li>' +
				'<li class="gnb_3 mb_gnb">' +
					'<a href="wifi_status.html">WiFi</a>' +
					'<div class="s_gnb">' +
						'<ul>' +
                            '<li><a href="wifi_status.html">WiFi 상태 정보</a></li>' +
                            '<li><a href="wifi_setup.html">WiFi 설정</a></li>' +
                            '<li><a href="wifi_handover.html">핸드오버 설정</a></li>' +
                            '<li><a href="wifi_ap_scan.html">주변 AP 스캔</a></li>' +
							'<li><a href="wifi_channel_scan.html">채널 스캔</a></li>' +
							'<li><a href="wifi_wmm.html">WMM</a></li>' +
							'<li><a href="wifi_wps.html">WPS</a></li>' +
						'</ul>' +
					'</div>' +
				'</li>' +
				'<li class="gnb_4 mb_gnb">' +
					'<a href="nat_functions.html">NAT/라우터</a>' +
					'<div class="s_gnb">' +
						'<ul>' +
							'<li><a href="nat_functions.html">포트 포워드</a></li>' +
							'<li><a href="nat_vpn.html">VPN</a></li>' +
						'</ul>' +
					'</div>' +
				'</li>' +
				'<li class="gnb_5 mb_gnb">' +
					'<a href="security_mac_filter.html">보안기능</a>' +
					'<div class="s_gnb">' +
						'<ul>' +
                            '<li><a href="security_mac_filter.html">MAC 주소 필터링</a></li>' +
                            '<li><a href="secure_eth_ip_filter.html">IP/Port 필터링</a></li>' +
							'<li><a href="secure_dos.html">DoS 공격 방어</a></li>' +
							'<li><a href="secure_storm.html">Storm 제어</a></li>' +
                            '<li><a href="secure_sld.html">Self Loop Detect</a></li>' +
							'<li><a href="secure_web_mng.html">관리 웹 접속</a></li>' +
						'</ul>' +
					'</div>' +
				'</li>' +
				'<li class="gnb_6 mb_gnb">' +
					'<a href="special_ddns.html">특수기능</a>' +
					'<div class="s_gnb">' +
						'<ul>' +
                            '<li><a href="special_ddns.html">DDNS</a></li>' +
							'<li><a href="special_port_mirror.html">포트 미러링</a></li>' +
							'<li><a href="special_wifi_iptv.html">무선 IPTV</a></li>' +
							'<li><a href="special_wifi_easy_mesh.html">EasyMesh</a></li>' +
                            '<li><a href="special_internet_limitation.html">인터넷 접속제한 서비스</a></li>' +
                            '<li><a href="special_self_test.html">자가진단 기능</a></li>' +
                            '<li><a href="special_nas.html">NAS</a></li>' +
                            '<li><a href="special_wol.html">WOL</a></li>' +
						'</ul>' +
					'</div>' +
				'</li>' +
				'<li class="gnb_7 mb_gnb">' +
					'<a href="stat_system.html">통계</a>' +
					'<div class="s_gnb">' +
						'<ul>' +
                            '<li><a href="stat_system.html">시스템 통계</a></li>' +
                            '<li><a href="stat_wire.html">유선 통계</a></li>' +
                            '<li><a href="stat_wifi.html">WiFi 통계</a></li>' +
                            '<li><a href="stat_connection.html">커넥션 정보 통계</a></li>' +
						'</ul>' +
					'</div>' +
				'</li>' +
				'<li class="gnb_8 mb_gnb">' +
					'<a href="mng_system.html">관리</a>' +
					'<div class="s_gnb">' +
						'<ul>' +
                            '<li><a href="mng_system.html">시스템 관리</a></li>' +
							'<li><a href="mng_remote.html">원격서버 관리</a></li>' +
							'<li><a href="mng_smart_reset.html">스마트 리셋</a></li>' +
                            '<li><a href="mng_led.html">LED OFF</a></li>' +
                            '<li><a href="mng_syslog.html">시스템 로그</a></li>' +
						'</ul>' +
					'</div>' +
				'</li>' +
			'</ul>' +
		'</div>' +
	'</div>' +
'</div>');
	
}