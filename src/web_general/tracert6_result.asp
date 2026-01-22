<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html" charset="arial">
<link rel="stylesheet" href="/admin/reset.css">
<link rel="stylesheet" href="/admin/base.css">
<link rel="stylesheet" href="/admin/style.css">
<script type="text/javascript" src="share.js"></script>
<title>Tracert <% multilang(LANG_DIAGNOSTICS); %></title>
</head>

<script>
function $id(id) {
	return document.getElementById(id);
}

function refresh_result()
{
	var xhr2 = new XMLHttpRequest();

	if (xhr2.upload) {
		xhr2.open("POST", $id("result").action, true);
		xhr2.onload = function() {
			if (xhr2.readyState == xhr2.DONE) {
				if (xhr2.status == 200) {
					//console.log(xhr2.responseText);
					document.getElementById("traceInfo").innerHTML = xhr2.responseText;
				}
			}
		};
		xhr2.send();
	}
	//console.log("Timer again");
}

function cleanrTimer(){
	clearInterval(timer_id);
	window.location.replace('/tracert6.asp');
}

function on_init(){
	timer_id = setInterval("refresh_result()", 1000);
}
</script>

<body onload="on_init();">
<br>
<form id="result" action="/boaform/formTracertResult" method="POST" name="result">
<div align="left">
	<table id="traceInfo">
	</table>
	<br>
</div>
</form>
<br><br>
</body>
</html>
