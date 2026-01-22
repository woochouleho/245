<%SendWebHeadStr(); %>
<title><% multilang(LANG_IGMP_PROXY); %><% multilang(LANG_CONFIGURATION); %></title>

<SCRIPT>
var multicast_allow=0;
var robust_count=0;
var last_member_query_count=0;
var query_interval=0;
var query_response_interval=0;
var group_leave_delay=0;
var multi_wan_proxy=0;
var fast_leave=2;
var query_mode=0;
var query_version=0;
<% igmpproxyinit(); %>
function on_init()
{
	with(document.forms[0])
	{
		if(multicast_allow == 0)
			igmp_multicast_allow[0].checked = true;
		else
			igmp_multicast_allow[1].checked = true;
		igmp_robust_count.value = robust_count;
		igmp_last_member_query_count.value = last_member_query_count;
		igmp_query_interval.value = query_interval;
		igmp_query_response_interval.value = query_response_interval;
		igmp_group_leave_delay.value = group_leave_delay;
		igmp_fast_leave.value = fast_leave;
		igmp_query_mode.value = query_mode;
		igmp_query_version.value = query_version;
	}
	if(multi_wan_proxy == 1)
	{
	   document.getElementById("proxy_enable").style="display:none";
	   document.getElementById("proxy_intf").style="display:none";
	 }
	 else
	 {
	   document.getElementById("proxy_enable").style="display:";
	   document.getElementById("proxy_intf").style="display:";
	  }
	proxySelection();
}
function proxySelection()
{
	if (document.igmp.proxy[0].checked && multi_wan_proxy == 0) {
		document.igmp.proxy_if.disabled = true;
		document.igmp.igmp_multicast_allow[0].disabled = true;
		document.igmp.igmp_multicast_allow[1].disabled = true;
		document.igmp.igmp_robust_count.disabled = true;
		document.igmp.igmp_last_member_query_count.disabled = true;
		document.igmp.igmp_query_interval.disabled = true;
		document.igmp.igmp_query_response_interval.disabled = true;
		document.igmp.igmp_group_leave_delay.disabled = true;
		document.igmp.igmp_fast_leave.disabled = true;
		document.igmp.igmp_query_mode.disabled = true;
		document.igmp.igmp_query_version.disabled = true;
	}
	else {
		document.igmp.proxy_if.disabled = false;
		document.igmp.igmp_multicast_allow[0].disabled = false;
		document.igmp.igmp_multicast_allow[1].disabled = false;
		document.igmp.igmp_robust_count.disabled = false;
		document.igmp.igmp_last_member_query_count.disabled = false;
		document.igmp.igmp_query_interval.disabled = false;
		document.igmp.igmp_query_response_interval.disabled = false;
		document.igmp.igmp_group_leave_delay.disabled = false;
		document.igmp.igmp_fast_leave.disbled = false;
		document.igmp.igmp_query_mode.disbled = false;
		document.igmp.igmp_query_version.disbled = false;
	}
}

function on_submit()
{

	// igmp_robust_count
	// igmp_last_member_query_count
	// igmp_query_interval
	// igmp_query_response_interval
	// igmp_group_leave_delay
	if (checkDigit(document.igmp.igmp_robust_count.value) == 0) {
		alert("ERROR: Please check the value");
		document.igmp.igmp_robust_count.focus();
		return false;
	}
	
	if (checkDigit(document.igmp.igmp_last_member_query_count.value) == 0) {
		alert("ERROR: Please check the value");
		document.igmp.igmp_last_member_query_count.focus();
		return false;
	}

	if (checkDigit(document.igmp.igmp_query_interval.value) == 0) {
		alert("ERROR: Please check the value");
		document.igmp.igmp_query_interval.focus();
		return false;
	}

	if (checkDigit(document.igmp.igmp_query_response_interval.value) == 0) {
		alert("ERROR: Please check the value");
		document.igmp.igmp_query_response_interval.focus();
		return false;
	}

	if (checkDigit(document.igmp.igmp_group_leave_delay.value) == 0) {
		alert("ERROR: Please check the value");
		document.igmp.igmp_group_leave_delay.focus();
		return false;
	}

	with(document.forms[0]) {
		if (igmp_query_response_interval.value > 128) {
			alert('<% multilang(LANG_QUERY_RESPONSE_INTERVAL_NOT_SUPPORT_128); %>');
			return false;
		}
		else {
			postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
			return true;
		}
	}
}
</SCRIPT>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_IGMP_PROXY); %> <% multilang(LANG_CONFIGURATION); %></p>
	<p class="intro_content"> <% multilang(LANG_IGMP_PROXY_ENABLES_THE_SYSTEM_TO_ISSUE_IGMP_HOST_MESSAGES_ON_BEHALF_OF_HOSTS_THAT_THE_SYSTEM_DISCOVERED_THROUGH_STANDARD_IGMP_INTERFACES_THE_SYSTEM_ACTS_AS_A_PROXY_FOR_ITS_HOSTS_WHEN_YOU_ENABLE_IT_BY_DOING_THE_FOLLOWS); %>:
    <br>. <% multilang(LANG_ENABLE_IGMP_PROXY_ON_WAN_INTERFACE_UPSTREAM_WHICH_CONNECTS_TO_A_ROUTER_RUNNING_IGMP); %>
    <br>. <% multilang(LANG_ENABLE_IGMP_ON_LAN_INTERFACE_DOWNSTREAM_WHICH_CONNECTS_TO_ITS_HOSTS); %></p>
</div>

<form action=/boaform/formIgmproxy method=POST name="igmp">
<div class="data_common data_common_notitle">
	<table>
		<tr id="proxy_enable">
			<th><% multilang(LANG_IGMP_PROXY); %>:</th>
			<td>
				<input type="radio" value="0" name="proxy" <% checkWrite("igmpProxy0"); %> onClick="proxySelection()"><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
				<input type="radio" value="1" name="proxy" <% checkWrite("igmpProxy1"); %> onClick="proxySelection()"><% multilang(LANG_ENABLE); %>
			</td>
		</tr>
		<tr id="proxy_intf">
			<th><% multilang(LANG_PROXY_INTERFACE); %>:</th>
			<td>
				<select name="proxy_if" <% checkWrite("igmpProxy0d"); %>>
			  		<% if_wan_list("rt"); %>
				</select>
			</td>
		</tr>
		<tr id = "igmp_multicast_allow_id" style="display:none">
			<th><% multilang(LANG_MULTICAST_ALLOWED); %>:</th>
			<td>
				<input type="radio" value="0" name="igmp_multicast_allow"><% multilang(LANG_DISABLE); %>
				<input type="radio" value="1" name="igmp_multicast_allow"><% multilang(LANG_ENABLE); %>
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_IGMP_ROBUST_COUNT); %>:</th>
			<td>
				<input type="text" name="igmp_robust_count" size="10" maxlength="15">
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_IGMP_LAST_MEMBER_QUERY_COUNT); %>:</th>
			<td>
				<input type="text" name="igmp_last_member_query_count" size="10" maxlength="15">
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_IGMP_QUERY_INTERVAL); %>:</th>
			<td>
				<input type="text" name="igmp_query_interval" size="10" maxlength="15">(seconds)
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_IGMP_QUERY_RESPONSE_INTERVAL); %>:</th>
			<td>
				<input type="text" name="igmp_query_response_interval" size="10" maxlength="15">(*100ms)
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_IGMP_GROUP_LEAVE_DELAY); %>:</th>
			<td>
				<input type="text" name="igmp_group_leave_delay" size="10" maxlength="15">(ms)
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_IGMP_FAST_LEAVE); %>:</th>
			<td>
				<input type="radio" value="0" name="igmp_fast_leave"><% multilang(LANG_DISABLE); %>
				<input type="radio" value="1" name="igmp_fast_leave"><% multilang(LANG_ENABLE); %>
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_IGMP_QUERY_MODE); %>:</th>
			<td>
				<input type="radio" value="0" name="igmp_query_mode"><% multilang(LANG_QUERY_AUTO); %>
				<input type="radio" value="1" name="igmp_query_mode"><% multilang(LANG_QUERY_FORCE); %>
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_IGMP_QUERY_VERSION); %>:</th>
			<td>
				<input type="radio" value="2" name="igmp_query_version"><% multilang(LANG_QUERY_V2); %>
				<input type="radio" value="3" name="igmp_query_version"><% multilang(LANG_QUERY_V3); %>
			</td>
		</tr>
	</table>
</div>

<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" onClick="return on_submit()">
	<input type="hidden" value="/igmproxy.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
<script>
	ifIdx = <% getInfo("igmp-proxy-itf"); %>;
	if (ifIdx != 255)
		document.igmp.proxy_if.value = ifIdx;
	else
		document.igmp.proxy_if.selectedIndex = 0;
	on_init();
</script>
</form>
</body>

</html>
