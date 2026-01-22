<%SendWebHeadStr(); %>
<meta http-equiv=Refresh CONTENT="10">
<title>PON flow Statistics</title>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_PON_FLOW_STATISTICS); %></p>
</div>

<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th><% multilang(LANG_PON_FLOW_HW_COUNT); %>:</th>
			<td><% ponflowGetStatus("hw"); %></td>
		</tr>
		<tr>
			<th><% multilang(LANG_PON_FLOW_SW_COUNT); %>:</th>
			<td><% ponflowGetStatus("sw"); %></td>
		</tr>	
	</table>
</div>
</body>
</html>
