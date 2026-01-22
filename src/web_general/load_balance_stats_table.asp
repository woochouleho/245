<% SendWebHeadStr();%>
<title><% multilang(LANG_LOAD_BALANCE_STATS); %></title>

<script>
var AjaxReq = null;
var AjaxReqSend = 0;

function update_page()
{
	if (AjaxReq != null && AjaxReq.readyState==4 && AjaxReq.status==200)
	{
		var string_interface = AjaxReq.responseText;
		document.getElementById("interface_form").innerHTML = string_interface;	
		AjaxReqSend  = 0;
	}
}

function createRequest()
{
	var request = null;
	try { request = new XMLHttpRequest(); }
	catch (trymicrosoft)
	{
		try { request = new ActiveXObject("Msxml2.XMLHTTP"); }
		catch (othermicrosoft)
		{
			try { request = new ActiveXObject("Microsoft.XMLHTTP"); }
			catch (failed)
			{
				request = null;
			}
		}
	}
	if (request == null) alert("Error creating request object !");
	return request;
}

function send_request(url)
{
	if (AjaxReq == null) AjaxReq = createRequest();
	AjaxReq.open("GET", url, true);
	AjaxReq.onreadystatechange = update_page;
	AjaxReq.send(null);

	AjaxReqSend = 1;
}

function generate_random_str()
{
	var d = new Date();
	var str=d.getFullYear()+"."+(d.getMonth()+1)+"."+d.getDate()+"."+d.getHours()+"."+d.getMinutes()+"."+d.getSeconds();
	return str;
}

function update_state()
{
	if(AjaxReqSend == 0){
		send_request("load_balance_stats_url.asp?t="+generate_random_str());
	}
	
}

function refresh_interface_page()
{
        update_state();
        setInterval(update_state, 1000);
}
function on_init()
{
	refresh_interface_page();
}

</script>
</head>
<body onload="on_init();">
<form action=/boaform/formLdStats method=POST name="formLdStats">
	<div id = "interface_form">
	</div>
<div class="btn_ctl">
  <input type="hidden" value="/load_balance_stats.asp" name="submit-url">
  <input type="hidden" value="0" name="reset">
  <input type="hidden" name="postSecurityFlag" value="">
  </div>
</form>
<br><br>
</body>

</html>

