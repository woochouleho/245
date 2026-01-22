<% SendWebHeadStr();%>
<title><% multilang(LANG_EASYMESH_CHANNEL_SCAN); %></title>
<style>
.on {display:on}
.off {display:none}

tbody.for_top_margin::before
{
  content: '';
  display: block;
  height: 20px;
}

</style>

<SCRIPT>

function loadInfo()
{
    if (null != document.getElementById("reload")) {
        setInterval(function(){ location.reload(true); }, 1000);
    }
}

function saveChanges(obj)
{
    if(null != document.getElementById("band_select_1") && document.getElementById("band_select_1").checked == true) {
        document.getElementById("band_select_1").value = "selected";
    }
    if(null != document.getElementById("band_select_2") && document.getElementById("band_select_2").checked == true) {
        document.getElementById("band_select_2").value = "selected";
    }
    return true;
}

</SCRIPT>
</head>

<body onload="loadInfo();">
<blockquote>
<h2>EasyMesh Channel Scan Result</h2>
<form action=/boaform/formChannelScan method=POST name="MultiAP">
<% showChannelScanResults(); %>
<input type="hidden" value="/multi_ap_channel_scan.asp" name="submit-url">

</form>
</blockquote>
</body>

</html>