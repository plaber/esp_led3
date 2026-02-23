#include "sub_http.h"
#include "sub_httpjs.h"
#include "conf.h"
#include "sub_eep.h"
#include "sub_bmp.h"
#include "sub_gif.h"
#include "sub_jpg.h"
#include "sub_json.h"
#include "sub_led.h"

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
FtpServer ftpSrv;

extern String ssid[];
extern String pass[];
extern FS* fileSystem;
char texthtml[10] = "text/html";
char textplain[11] = "text/plain";
char acao[28] = "Access-Control-Allow-Origin";

unsigned char h2int(char c)
{
	if (c >= '0' && c <='9')
	{
		return((unsigned char)c - '0');
	}
	if (c >= 'a' && c <='f')
	{
		return((unsigned char)c - 'a' + 10);
	}
	if (c >= 'A' && c <='F')
	{
		return((unsigned char)c - 'A' + 10);
	}
	return(0);
}

String urldecode(String str)
{
	String encodedString = "";
	char c;
	char code0;
	char code1;
	for (int i =0; i < str.length(); i++)
	{
		c=str.charAt(i);
		if (c == '+')
		{
			encodedString+=' ';
		}
		else if (c == '%')
		{
			i++;
			code0 = str.charAt(i);
			i++;
			code1 = str.charAt(i);
			c = (h2int(code0) << 4) | h2int(code1);
			encodedString += c;
		}
		else
		{
			encodedString+=c;
		}
	yield();
	}
	return encodedString;
}

String urlencode(String str)
{
	String encodedString="";
	char c;
	char code0;
	char code1;
	char code2;
	for (int i =0; i < str.length(); i++)
	{
		c=str.charAt(i);
		if (c == ' ')
		{
			encodedString += '+';
		}
		else if (isalnum(c))
		{
			encodedString += c;
		}
		else
		{
			code1 = (c & 0xf) + '0';
			if ((c & 0xf) >9)
			{
				code1=(c & 0xf) - 10 + 'A';
			}
			c = (c >> 4) & 0xf;
			code0 = c + '0';
			if (c > 9)
			{
				code0 = c - 10 + 'A';
			}
			code2 = '\0';
			encodedString += '%';
			encodedString += code0;
			encodedString += code1;
			//encodedString+=code2;
		}
		yield();
	}
	return encodedString;
}

static const char view_svg[] PROGMEM = "<?xml version=\"1.0\" ?><svg viewBox=\"0 0 20 20\" xmlns=\"http://www.w3.org/2000/svg\"><path d=\"M.2 10a11 11 0 0 1 19.6 0A11 11 0 0 1 .2 10zm9.8 4a4 4 0 1 0 0-8 4 4 0 0 0 0 8zm0-2a2 2 0 1 1 0-4 2 2 0 0 1 0 4z\"/></svg>";
static const char noview_svg[] PROGMEM = "<?xml version=\"1.0\" ?><svg viewBox=\"0 0 20 20\" xmlns=\"http://www.w3.org/2000/svg\"><path d=\"M12.81 4.36l-1.77 1.78a4 4 0 0 0-4.9 4.9l-2.76 2.75C2.06 12.79.96 11.49.2 10a11 11 0 0 1 12.6-5.64zm3.8 1.85c1.33 1 2.43 2.3 3.2 3.79a11 11 0 0 1-12.62 5.64l1.77-1.78a4 4 0 0 0 4.9-4.9l2.76-2.75zm-.25-3.99l1.42 1.42L3.64 17.78l-1.42-1.42L16.36 2.22z\"/></svg>";

static const char content_root[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<title>poi</title>
<meta charset='UTF-8'>
<style>
button {margin:10px 20px}
table, td {border:1px solid #ccc}
td {padding: 0 10px}
.pwd {display:inline-block;width:20px;height:20px;background:url(/view.svg) 0 0 no-repeat;}
.pwd.view {background:url(/noview.svg) 0 0 no-repeat;}
</style>
</head>

<body>
<a href='/'>main</a>   
<a href='/files'>files</a>   
<a href='/progs'>progs</a>   
<a href='/prog'>prog</a>   
<a href='/lis'>lis</a>   
<a href='/config'>conf</a>

<br><br>

<table border=1><caption>LED <span id='ver'></span></caption>
	<tr><td>ip</td><td id=ip></td></tr>
	<tr><td>mac</td><td id=mac></td></tr>
	<tr><td>maca</td><td id=maca></td></tr>
	<tr><td>vcc</td><td id=vcc onclick="r('vcc')"></td></tr>
	<tr><td>heap</td><td id='heap' onclick="r('heap')"></td></tr>
	<tr><td>prog</td><td><select id='progs'></select> <input type='button' value='ok' onclick="r('prg',vl('progs'));"></td></tr>
	</table><br><br>

<table border=1><caption>управление</caption>
	<tr><td>задержка</td>
		<td><button onclick="r('wait','m')">-</button></td>
		<td id=wait></td>
		<td><button onclick="r('wait','p')">+</button></td></tr>
	<tr><td>яркость</td>
		<td><button onclick="r('brgn','m')">-</button></td>
		<td id=brgn></td>
		<td><button onclick="r('brgn','p')">+</button></td></tr>
	<tr><td>режим</td>
		<td><button onclick="r('mode','3')">файлы</button></td>
		<td><button onclick="r('mode','4')">программа</button></td>
		<td id=mode></td></tr>
	<tr><td>запуск</td>
		<td><button onclick="r('go')">go</button></td>
		<td><button onclick="r('stp')">stop</button></td>
		<td id=go></td></tr>
	<tr><td>настройки</td>
		<td><button onclick="r('cmt')">сохранить</button></td>
		<td><button onclick="if(confirm('сброс?'))r('rst')">сбросить</button></td>
		<td><span id=rst></span> <span id=cmt></span></td></tr>
	<tr><td>перезагрузка</td>
		<td colspan=2><button onclick="r('restart')">сбросить</button></td>
		<td></td></tr>
</table><br><br>

<table border=1><caption>Подключаться к точке</caption>
	<tr><td id=wfaps></td></tr>
	<tr><td>pass: <input id=pass type=password>
	<a href='#' class=pwd onclick='return sh_pw(this);'></a><br>
	<tr><td><label><input type=radio name=sidx value=1 checked>точка 1</label>
			<label><input type=radio name=sidx value=2>точка 2</label>
	<input type=button value=сохранить onClick="r('ssid', vr('ssid')+'&p='+vl('pass')+'&i='+vr('sidx'))"> 
	<input type=button value=reset onClick="r('ssid', 'spiffs&p=spiffs&i='+vr('sidx'))"><br><span id='ssid'></span></td></tr>
</table><br>

<table border=1><caption>Название устройства</caption>
	<tr><td><input id=wprefv maxlength=15 onkeyup='this.value = this.value.replace(/[^A-Za-z0-9_]/g, "")'> 
	<input type=button value='сохранить' onClick="r('wpref',vl('wprefv'))"><br><span id=wpref></span></td></tr>
</table><br>

<table border=1><caption>Адрес mac_ap для синхронизации</caption>
	<tr><td><input id=mcap> 
	<input type=button value='сохранить' onClick="r('macs',vl('mcap'))"><br><span id=macs></span></td></tr>
	<tr><td><hr></td></tr>
	<tr><td>Связать устройства: <input type=button value=sync onclick="r('macun','echo')"> 
			<input type=button value=reset onclick="r('macun','unsn')"> 
			<span id=macun></span></td></tr>
</table><br>

<span onclick="r('conf')">show conf  </span><span id=conf></span><br><br>

<a href='/update'>обновление</a>   <a href='/fs.bin'>fs.bin</a><br><br>

<script>
function vl(id){var el=document.getElementById(id); return el.value;}
function vr(nm){var ra=document.getElementsByName(nm);for(var i=0;i<ra.length;i++)if(ra[i].checked) return ra[i].value;}
function r(p,v){
	fetch('/req?'+p+'='+(v?v:'1'))
	.then((response) => {return response.text();})
	.then((data) => {var e=document.getElementById(p); if(e) e.innerHTML = data;});
}
function load(v){
	for(var key in v){
		var el = document.getElementById(key);
		if(!el) continue;
		if(el.tagName=='SELECT'){
			ms = v[key].split(',');
			for(var i=0;i<ms.length;i++){
				el.options[el.options.length] = new Option(ms[i], i+1);
				if(ms[i]==v.prog) {el.selectedIndex = i;}
			}
		}
		else el.innerHTML = v[key];
	}
}
function sh_pw(t){
	var i=document.getElementById('pass');
	if (i.getAttribute('type')=='password'){
		t.classList.add('view');
		i.setAttribute('type','text');
	} else {
		t.classList.remove('view');
		i.setAttribute('type','password');
	}
	return false;
};
fetch('/req?ver=1&ip=1&mac=1&maca=1&vcc=1&wait=0&brgn=0&mode=0&heap=1&prog=1&progs=1')
		.then((response) => {return response.json();})
		.then((data) => {load(data);r('wfaps');})
</script>
</body></html>
)=====";

void handleRoot()
{
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, texthtml, content_root);
}

bool checkString(String a)
{
	int l = a.length();
	for (int i = 0; i < l; i++)
	{
		if (a.charAt(i) > 127) return true;
	}
	return false;
}

static const char content_files1[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<title>poi files</title><meta charset='UTF-8'>
<style>
input {margin:0 5px 5px 0;}
tr.cn {display: none}
td {padding: 0 10px}
table.pc tr td {width: 3px;height: 3px;padding: 0 0px}
img.jpg, img.gif {max-height: 150px}
</style>
</head>
<body>
<a href='/'>main</a>   
<a href='/files'>files</a>   
<a href='/progs'>progs</a>   
<a href='/prog'>prog</a>   
<a href='/lis'>lis</a>
)=====";

static const char content_files2[] PROGMEM = R"=====(
 
<form method='GET' onsubmit="return confirm('Warning. Delete all files?');" style='display:inline;'>
<input type=submit value=format name=format> <input type=submit value='del converted' name=delbma>
</form><br><br>

<form method='POST' enctype='multipart/form-data' action='/load'>
Load <input type=file name=data><input type=submit value=load><br>
Many <input type=file id=files multiple><input type=button value=load onclick='send()'> <span id=q>ok</span><br>
</form><br>

<script>
function del(name, t, rid){
	var qwe = confirm('delete ' + name + '?');
	if(qwe) fetch('/del?f='+encodeURI(name))
		.then((response) => {return response.text();})
		.then((data) => {
			alert(data);
			if(data.indexOf('<br>ok')!=-1){
				var el=document.getElementById('row_'+rid+'a');
				if(el) el.remove();
				document.getElementById('row_'+rid).remove();
			}
	})
}
function showc(){
	var el = document.getElementsByClassName('cn');
	for(var i=0;i<el.length;i++) {
		el[i].style.display = 'table-row';
		var im = el[i].getElementsByTagName('IMG')[0];
		im.setAttribute('src', im.getAttribute('data-src'));
	}
}
var onErr=function(err){console.warn(err);return new Response('error');}
async function send(){
	var fl=document.getElementById('files');
	var ans=document.getElementById('q');
	for(var i=0; i<fl.files.length;i++){
		var fd=new FormData();
		fd.append('data',fl.files[i]);
		let r=await fetch('/load',{method:'POST',body:fd}).catch(onErr);
		let t=await r.text();
		console.log(t);
		ans.innerHTML=t;
	}
	ans.innerHTML='finish';
	setTimeout('location.reload()',1000);
}
async function savep(){
	var fd=new FormData(document.forms[2]);
	var p = /^[a-zA-z0-9_]+$/;
	if(!p.test(fd.get('progn'))){alert('wrong prog name');return;}
	if(!fd.get('psel')){alert('no pics selected');return;};
	let r=await fetch('/progs',{method:'POST',body:fd}).catch(onErr);
	let t=await r.text();
	alert(t);
}
function resetp(){
	var el = document.getElementsByName('psel');
	for(var i=0; i<el.length; i++) el[i].checked=false;
}
</script>

<form>
<input type=button value='show converted' onClick='showc()'>   
<input name=prognm placeholder='prog name' maxlength=20 onkeyup='this.value = this.value.replace(/[^A-Za-z0-9_]/g, "")'>
<input type=button value='save prog' onclick='savep()'>   
<input type=button value='reset sel' onclick='resetp()'>   
)=====";

void handleFiles()
{
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	bool showinfo = false;
	if (server.args() > 0)
	{
		String fn = server.argName(0);
		if (fn == "format" || fn == "delbma")
		{
			String ans = get_answ(fn, "1");
			server.send(200, texthtml, F("<a href='/files'>back</a><br>") + ans);
			return;
		}
	}
	server.send(200, texthtml, content_files1);
	FSInfo fs_info;
	fileSystem->info(fs_info);
	server.sendContent(F("<br><br>space:") + String(fs_info.usedBytes, DEC) +F(" / ") + String(fs_info.totalBytes, DEC) + F(" [free ") +  String(fs_info.totalBytes - fs_info.usedBytes, DEC) + F("]"));
	server.sendContent(content_files2);
		if (server.args() > 0)
	{
		if (server.argName(0) == "debug")
		{
			showinfo = true;
			server.sendContent(F("<a href='/files?'>no debug</a>"));
		}
		else
		{
			server.sendContent(F("<a href='/files?debug=1'>debug</a>"));
		}
	}
	else
	{
		server.sendContent(F("<a href='/files?debug=1'>debug</a>"));
	}
	server.sendContent(F("<table border=1><tr><td>#</td><td>del</td><td>name</td><td>size</td><td>sel</td><td>pic</td><td>h</td><td>w</td><td>bits</td><td>tp</td><td>decode</td></tr>"));
	Dir d = fileSystem->openDir("/");
	int nr = 1;
	while (d.next())
	{
		String fileName = d.fileName();
		size_t fileSize = d.fileSize();
		String ans = F("<tr id='row_");
		ans += String(nr, DEC);
		ans += fileName.endsWith(exbma) ? F("a' class='cn'") : F("' ");
		ans += F("><td>");
		ans += String(nr, DEC);
		ans += F("</td><td>");
		ans += F("<input type='button' value='del' onClick='del(\"");
		ans += fileName;
		ans += F("\",this,\"");
		ans += fileName.endsWith(exbma) ? String(nr, DEC) + "a" : String(nr, DEC);
		ans += F("\")'</td>");
		ans += F("<td");
		ans += checkString(fileName) ? F(" style='background:#fcc;'") : F(">");
		ans += F("<a href='");
		ans += fileName;
		ans += F("'>");
		ans += fileName;
		ans += F("</a></td><td");
		ans += fileSize == 0 ? F(" style='background:#fcc;'>") : F(">");
		ans += String(fileSize);
		ans += F("</td><td>");
		if (bmp_check(fileName))
		{
			ans += F("<input type='checkbox' name='psel' value='");
			ans += fileName;
			ans += F("'>");
		}
		ans += F("</td>");
		if (!fileName.endsWith(exbma)) nr++;
		if (fileName.endsWith(exbmp) || fileName.endsWith(exbma))
		{
			ans += F("<td ");
			ans += fileName.endsWith(exbma) ? F("style='background:#ccc;'") : F("");
			ans += fileName.endsWith(exbma) ? F("><img data-src='") : F("><img src='");
			ans += fileName;
			ans += F("'></td><td>");
			if (showinfo)
			{
				struct bmpheader hd = bmp_header(fileName);
				ans += String(hd.h, DEC);
				ans += F("</td><td>");
				ans += String(hd.w, DEC);
				ans += F("</td><td>");
				ans += String(hd.bits, DEC);
				ans += F("</td><td>");
				ans += String(hd.bminfo);
				ans += F(".");
				ans += String(hd.compres, DEC);
				ans += F("</td><td>");
			}
			else
			{
				ans += F("</td><td></td><td></td><td></td><td>");
			}
			if (fileName.endsWith(exbmp))
			{
				ans += F("<a href='/dec?f=");
				ans += urlencode(fileName);
				ans += F("' target='_blank'>decode</a>");
			}
			ans += F("</td>");
		}
		if (fileName.endsWith(exgif))
		{
			ans += F("<td><img src='");
			ans += fileName;
			ans += F("' class='gif'></td><td>");
			if (showinfo)
			{
				struct gifheader hd = gif_header(fileName, false);
				char mod[4] = {hd.mod[0], hd.mod[1], hd.mod[2], 0};
				ans += String(hd.h, DEC);
				ans += F("</td><td>");
				ans += String(hd.w, DEC);
				ans += F("</td><td>");
				ans += String(hd.cdp + 1, DEC);
				ans += F("</td><td>");
				ans += String(mod);
				ans += F("</td><td>");
			}
			else
			{
				ans += F("</td><td></td><td></td><td></td><td>");
			}
			ans += F("<a href='/dec?f=");
			ans += urlencode(fileName);
			ans += F("' target='_blank'>decode</a></td>");
			
		}
		if (fileName.endsWith(exjpg))
		{
			ans += F("<td><img src='");
			ans += fileName;
			ans += F("' class='jpg'></td><td>");
			if (showinfo)
			{
				struct jpgheader hd = jpg_header(fileName, false);
				ans += String(hd.h, DEC);
				ans += F("</td><td>");
				ans += String(hd.w, DEC);
				ans += F("</td><td>-</td><td>");
				ans += String(hd.s, DEC);
				ans += F(" [");
				ans += String(hd.h * 1.0 / hd.s, 2) + "x" + String(hd.h * 1.0 / hd.s, 2) + "][" + String(hd.nd) + "b]";
				ans += F("</td><td>");
			}
			else
			{
				ans += F("</td><td></td><td></td><td></td><td>");
			}
			ans += F("<a href='/dec?f=");
			ans += urlencode(fileName);
			ans += F("' target='_blank'>decode</a></td>");
		}
		server.sendContent(ans);
		server.sendContent(F("</tr>\n"));
	}
	server.sendContent(F("</table></form>\n</body>\n</html>"));
}

static const char content_filesap1[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<title>poi filesap</title><meta charset='UTF-8'>
<style>
table {margin:0 auto;}
#tblf tr td {padding:0 5px;vertical-align:middle;}
</style>
</head>
<body>
<table style='background:#ffc;' border=1 id=tblf>
<tr><td>#</td><td>name</td><td>img</td></tr>
)=====";

static const char content_filesap2[] PROGMEM = R"=====(
</table>
<script>
function showAndroidToast(toast){
	if(typeof Android!=='undefined' && Android!==null){
		Android.showToast(toast);
	} else {
		alert('Click this in android App!');
	}
}
</script>
</body></html>
)=====";

void handleFilesap()
{
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, texthtml, content_filesap1);

	Dir dir = fileSystem->openDir("/");
	int filen = 1;
	while (dir.next())
	{
		String fileName = dir.fileName();
		if (bmp_check(fileName))
		{
			struct bmpheader hd = bmp_header(fileName);
			server.sendContent(F("<tr><td>") + String(filen, DEC) + F("<td onClick='showAndroidToast(\"") + fileName + "&" + String(filen, DEC) + F("\")'>") + fileName + F("</td>"));
			server.sendContent(F("<td style='height:") + String(hd.h + 2, DEC) + F("px'><img src='") + fileName + F("'></td>"));
			filen++;
		}
		server.sendContent(F("</tr>\n"));
	}
	server.sendContent(content_filesap2);
}

void handlePics()
{
	Dir dir = fileSystem->openDir("/");
	int filen = 0;
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.sendHeader(acao, "*");
	server.send(200, textplain, "");
	while (dir.next())
	{
		String fileName = dir.fileName();
		if (bmp_check(fileName) || (server.args() == 1 && server.argName(0) == "all"))
		{
			if (filen)
				server.sendContent("\n" + fileName);
			else
				server.sendContent(fileName);
			filen++;
		}
	}
}

void handleDec()
{
	if (server.args() > 0)
	{
		String f = server.arg(0);
		server.setContentLength(CONTENT_LENGTH_UNKNOWN);
		server.send(200, texthtml, F("<style>body {font-family:monospace}\nimg{width:") + String(conf.leds * 3) + F("px}</style>"));
		String buf;
		if (fileSystem->exists(f) && f.endsWith(exjpg))
		{
			jpgheader ans = jpg_header(f, false);
			buf = F("<br>file found");
			buf += F("<br>width: ");
			buf += String(ans.w, DEC);
			buf += F("<br>height: ");
			buf += String(ans.h, DEC);
			buf += F("<br>need space: ");
			buf += String(ans.w * ans.h * 2 / ans.s / ans.s, DEC);
			buf += F("<br>");
			server.sendContent(buf);
			led_clear();
			jpg_header(f, true);
			String newname = f.substring(0, f.length() - 3);
			server.sendContent(F("<img src='") + newname + F("bma'>"));
		}
		else if (fileSystem->exists(f) && f.endsWith(exgif))
		{
			gifheader ans = gif_header(f, false);
			buf = F("<br>file found");
			buf += F("<br>width: ");
			buf += String(ans.w, DEC);
			buf += F("<br>height: ");
			buf += String(ans.h, DEC);
			buf += F("<br>");
			server.sendContent(buf);
			led_clear();
			gif_header(f, true);
			String newname = f.substring(0, f.length() - 3);
			server.sendContent(F("<img src='") + newname + F("bma'>"));
		}
		else if (fileSystem->exists(f) && f.endsWith(exbmp))
		{
			bmpheader ans = bmp_header(f);
			buf = F("<br>file found");
			buf += F("<br>width: ");
			buf += String(ans.w, DEC);
			buf += F("<br>height: ");
			buf += String(ans.h, DEC);
			buf += F("<br>");
			server.sendContent(buf);
			led_clear();
			bmp_rotate(f);
			String newname = f.substring(0, f.length() - 3);
			server.sendContent(F("<img src='") + newname + F("bma'>"));
		}
		else
		{
			server.sendContent(F("no file ") + f);
		}
	}
	else
	{
		server.send(200, texthtml, F("no arg"));
	}
}

String getContentType(String filename)
{
	if (server.hasArg("download")) return F("application/octet-stream");
	else if (filename.endsWith(".htm") || filename.endsWith(".html")) return texthtml;
	else if (filename.endsWith(extxt)) return textplain;
	else if (filename.endsWith(".css")) return F("text/css");
	else if (filename.endsWith(".js" )) return F("application/javascript");
	else if (filename.endsWith(".png")) return F("image/png");
	else if (filename.endsWith(exgif)) return F("image/gif");
	else if (filename.endsWith(exjpg)) return F("image/jpeg");
	else if (filename.endsWith(exbmp) || filename.endsWith(exbma)) return F("image/bmp");
	else if (filename.endsWith(".ico")) return F("image/x-icon");
	else if (filename.endsWith(".xml")) return F("text/xml");
	else if (filename.endsWith(".pdf")) return F("application/x-pdf");
	else if (filename.endsWith(".zip")) return F("application/x-zip");
	else if (filename.endsWith(".gz" )) return F("application/x-gzip");
	return textplain;
}

bool handleFileRead(String path)
{
	path = urldecode(path);
	Serial.print(F("handleFileRead: "));
	Serial.println(path);
	String contentType = getContentType(path);
	if (!fileSystem->exists(path))
	{
		path = path + F(".gz");
	}
	if (fileSystem->exists(path))
	{
		long ms = millis();
		File file = fileSystem->open(path, "r");
		server.sendHeader(acao, "*");
		server.streamFile(file, contentType);
		file.close();
		Serial.println(String(millis() - ms, DEC) + "ms");
		return true;
	}
	return false;
}

void handleFsRead()
{
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, F("application/octet-stream"), "");
	uint32_t ubuf[256];
	char cbuf[1024];
	for (int i = 0x100000; i < 0x3FA000; i+= 0x400)
	{
		spi_flash_read(i, ubuf, sizeof(ubuf));
		memcpy(cbuf,ubuf,1024);
		server.sendContent_P(cbuf, 1024);
	}
}

void handleNotFound() {
	if (!handleFileRead(server.uri()))
	{
		server.send(404, texthtml, "<a href='/'>Not found</a>");
	}
}

void handleReq()
{
	if (server.args() == 3 && server.argName(0) == F("ssid") && server.argName(1) == "p" && server.argName(2) == "i")
	{
		int idx = server.arg(2).toInt();
		if (idx == 1)
		{
			ssid[1] = server.arg(0);
			pass[1] = server.arg(1);
			json_save();
			if(server.arg(0) == F("spiffs"))
				server.send(200, textplain, F("ssid 1 reset ok"));
			else
				server.send(200, textplain, F("Saved 1 ssid:") + ssid[1] + F(" pass: <span title='") + pass[1] + F("'>*****</span>"));
		}
		else if (idx == 2)
		{
			ssid[2] = server.arg(0);
			pass[2] = server.arg(1);
			json_save();
			if(server.arg(0) == F("spiffs"))
				server.send(200, textplain, F("ssid 2 reset ok"));
			else
				server.send(200, textplain, F("Saved 2 ssid:") + ssid[2] + F(" pass: <span title='") + pass[2] + F("'>*****</span>"));
		} else {
			server.send(200, textplain, F("не выбрана точка 1 или 2"));
		}
	}
	else if (server.args() == 1)
	{
		String ans = get_answ(server.argName(0), server.arg(0));
		server.send(200, textplain, ans);
		
		if (ans == F("restart"))
		{
			led_clear();
			delay(500);
			ESP.restart();
		}
	}
	else
	{
		String ans = "{";
		for (int i = 0; i < server.args(); i++)
		{
			ans += "\"" + server.argName(i) + "\":\"";
			ans += get_answ(server.argName(i), server.arg(i));
			if (i < server.args() - 1) ans += "\","; else ans += "\"";
		}
		ans += "}";
		server.send(200, textplain, ans);
	}
}

static const char content_lis[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<title>poi lis</title>
<meta charset='UTF-8'>
</head>
<body>
<a href='/'>main</a>   
<a href='/files'>files</a>   
<a href='/progs'>progs</a>   
<a href='/prog'>prog</a>   
<a href='/lis'>lis</a>

   max:<span id=max></span><br><br>

<canvas id=can></canvas>
<script>
draw = function(e,t){
	var r=document.getElementById(t);
	r.width=1000;
	r.height=500;
	if(r){
		for(var n,o,l=e.length,i=parseInt(r.width,10),f=parseInt(r.height,10),a=i/(l-1),s=r.getContext('2d'),d=null,h=function(e){
				for(var t,r,n,o=[],l=e.length,i=0;i<l;i++)'number'==typeof(n=e[i])&&('number'!=typeof t&&(r=t=n),r=n<(t=t<n?n:t)?n:r,e[i]);
				var f=t-r;
				for(i=0;i<l;i++)'number'==typeof(n=e[i])?o.push({val:2*((n-r)/f-.5),data:n,index:i}):o.push(null);
				return o
			}(e),v=0;v<l;v++)(o=h[v])&&(d=d||o,n=o);
		if(n){
			for(s.save(),s.fillStyle='#f2f2f2',s.lineWidth='3',s.fillRect(0,0,i,f),s.restore(),s.beginPath(),v=1;v<l;v++)s.moveTo(v*a,0),s.lineTo(v*a,f);
			for(s.save(),s.strokeStyle='#DDD',s.stroke(),s.restore(),s.beginPath(),s.moveTo(d.index*a,f),v=0;v<l;v++)(o=h[v])&&s.lineTo(o.index*a,-o.val*f*.8/2+f/2);
			s.lineTo(n.index*a,f),s.save(),s.fillStyle='rgba(8,106,253,.4)',s.strokeStyle='#086afc',s.lineWidth='2',s.stroke(),s.fill(),s.restore(),s.save(),
			s.strokeStyle='#666',s.lineWidth='3',s.strokeRect(0,0,i,f),s.restore()
		}
	}
};
function am(ar){return Math.max.apply(null, ar);}
)=====";

void handleLis()
{
	extern int32_t *lisd;
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, texthtml, content_lis);
	if (stat.lis_found == false)
	{
		server.sendContent(F("document.write('<br>Lis3d Chip could not initialize');"));
	}
	else
	{
		server.sendContent(F("var d=["));
		for (int i = 0; i < 1000; i += 10)
		{
			server.sendContent(
				String(lisd[i+0], DEC) + F(",") + String(lisd[i+1], DEC) + F(",") +
				String(lisd[i+2], DEC) + F(",") + String(lisd[i+3], DEC) + F(",") +
				String(lisd[i+4], DEC) + F(",") + String(lisd[i+5], DEC) + F(",") +
				String(lisd[i+6], DEC) + F(",") + String(lisd[i+7], DEC) + F(",") +
				String(lisd[i+8], DEC) + F(",") + String(lisd[i+9], DEC) + F(",")
			);
		}
		server.sendContent(F("]\ndraw(d,'can');\ndocument.getElementById('max').innerHTML=am(d);"));
	}
	server.sendContent(F("\n</script>\n</body>\n</html>"));
}

static const char content_prog[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<title>poi prog</title><meta charset='UTF-8'>
<script src='prog.js'></script>
<style>img,input,select {margin:0 5px 5px 0;}</style>
</head>
<body>
<a href='/'>main</a>   
<a href='/files'>files</a>   
<a href='/progs'>progs</a>   
<a href='/prog'>prog</a>   
<a href='/lis'>lis</a>
<br><br>

mp3 <input type='file' onchange='ws.loadBlob(this.files[0]);'>
<div id='wf'></div>
<input type='button' value='play' onClick='ws.play(0)'><input type='button' value='pause' onClick='ws.playPause()'>
zoom <input type='button' value='200x' onClick='ws.zoom(200);'><input type='button' value='1x' onClick='ws.zoom(1)'>
prog <input type='button' value='load' onClick='lp()'><input type='button' value='generate' onClick='genp()'>
pics <input type='button' value='clear' onClick='if(confirm("del all?")) ws.clearRegions()'><label><input type='checkbox' id='delrc'>del by click</label>
<div style='width:100%;overflow:scroll;'><table id='pcs'></table></div><br>

<select id='progs' onchange='loadf(this.value)'></select><input type='button' value='del' onclick='delp()'>
<br>
<form method='POST'>
name <input name='progn' id='progn' maxlength=20 onkeyup='this.value = this.value.replace(/[^A-Za-z0-9_.]/g, "")'><br>
<textarea name='prog' id='prog' rows=20 cols=30></textarea><br>
<input type='button' value='save' onclick='savep()'>
</form>

<script>
function loads(v){
	var a = document.getElementById('progs');
	for(var i = 0; i < a.length; i++) if(a[i].value == v) return;
	a.innerHTML += '<option>' + v + '</option>';
}
async function loadf(v){
	if (v=='0') v = document.getElementById('progs').value;
	if (!v) return;
	document.getElementById('progn').value = v;
	var a = await fetch('prog_'+v+'.txt');
	var b = await a.text();
	document.getElementById('prog').value = b;
	if (document.getElementById('progs').value != document.getElementById('progn').value)
		document.getElementById('progs').value = document.getElementById('progn').value;
}
fetch('/pics').then((r) => {return r.text();}).then((d) => {
	var f = d.split('\n'), rs = '';
	for (var i = 0; i < f.length; i++) rs += '<td><img src="' + f[i] + '" ondblclick="addr(this.src)"></td>';
	document.getElementById('pcs').innerHTML = '<tr>' + rs + '</tr>';
});
var ws = WaveSurfer.create({
	container: '#wf',
	plugins: [
		WaveSurfer.regions.create({})
	]
});
ws.on('region-click', function(region) {if(document.getElementById('delrc').checked) region.remove();});

var rg = 0, rgs = {}, p = document.getElementById('prog');
function lp(){
	ws.clearRegions();
	var t = p.value;
	var s = t.split('\n');
	var pr = [];
	for(var i = 0; i < s.length-1; i++){
		var r = s[i].split(' ');
		if(r[0].indexOf('stop') == 0){
			pr.push(r[0]);
		} else {
			pr.push(r[0]);
			pr.push(r[1]);
		}
	}
	var prev = 0;
	for(var i=0; i < pr.length; i+=2){
		ws.addRegion({
			id: 'reg'+rg,
			start: (prev/1000),
			end: (prev/1000)+0.5
		});
		prev += parseInt(pr[i+1]);
		var nregs = [].filter.call(document.getElementsByTagName('region'), el => el.dataset['id'] == 'reg'+rg);
		var stopImg = "url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAcAAAAgCAIAAABRmfUKAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsMAAA7DAcdvqGQAAABYSURBVChTtYzBDcAgDAMzSJ/sv1l3IMiREZYL/XDKg3NM4sD7tJwSQF8W2iLmAvEXtL67MEGLUylQB7dST3a1Tl8Wt9IkXSMPiua7PAafKaYcqAOf/iCiA+ltOHHSEMWvAAAAAElFTkSuQmCC')";
		nregs[0].style.background = pr[i].indexOf("stop") == 0 ? stopImg : "url('"+pr[i]+"')";
		nregs[0].style.backgroundRepeat = 'repeat-x';
		rgs['reg'+rg] = pr[i];
		rg++;
	}
}
function sof(a,b) {if (a[0] === b[0]) return 0; else return (a[0] < b[0]) ? -1 : 1;}
function genp()
{
	p.value = '';
	var out = '', pics = [];
	for(var k in ws.regions.list){
		var r = ws.regions.list[k];
		pics.push([parseInt(r.start*1000), (rgs[r.id].split('/').pop())]);
	}
	pics.sort(sof);
	pics.push([parseInt(ws.getDuration()*1000), '/kek']);
	for(var i = 0; i < pics.length - 1; i++)
		if(pics[i][1].indexOf("/stop") == 0)
			out += "stop\n";
		else
			out += pics[i][1] + ' ' + (pics[i+1][0] - pics[i][0]) + "\n";
	p.value = out;
}
function ltime(){
	var t = 0;
	for(var k in ws.regions.list){
		var r = ws.regions.list[k];
		if( r.end > t ) t = r.end;
	}
	return t;
}
function addr(p){
	ws.addRegion({
		id: 'reg'+rg,
		start: ltime(),
		end: ltime()+0.5
	});
	var nregs = [].filter.call(document.getElementsByTagName('region'), el => el.dataset['id'] == 'reg'+rg);
	nregs[0].style.background = "url('"+p+"')";
	nregs[0].style.backgroundRepeat = 'repeat-x';
	rgs['reg'+rg] = p;
	rg++;
}
async function savep(){
	var fd=new FormData(document.forms[0]);
	var p = /^[a-zA-z0-9_]+$/;
	if(!p.test(fd.get('progn'))){alert('wrong prog name');return;}
	let r=await fetch('/prog',{method:'POST',body:fd});
	let t=await r.text();
	loads(document.getElementById('progn').value);
	alert(t);
}
function delp(){
	var a = document.getElementById('progs');
	var v = a.value, i = a.selectedIndex;
	if (i == -1) return;
	var t = a.options[i].text;
	var q = confirm('delete '+t+' ?');
	if(q) fetch('/del?f='+encodeURI('prog_'+v+'.txt'))
		.then((response) => {return response.text();})
		.then((data) => {alert(data);if(data.indexOf('<br>ok')!=-1){a.remove(i); a.onchange();}
	})
}
)=====";

void handleProg()
{
	if (server.args() > 0)
	{
		String fname = F("/prog_") + server.arg(0).substring(0,20) + extxt;
		if (server.arg(0).startsWith("prog") && server.arg(0).endsWith(extxt))
		{
			fname = server.arg(0);
			if (fname.length() > 30) fname = fname.substring(0,25) + extxt;
		}
		File prgfile = fileSystem->open(fname, "w");
		prgfile.print(server.arg(1));
		prgfile.close();
		server.send(200, textplain, F("saved file: ") + fname + F("\r\n") + server.arg(1));
		bmp_max();
		return;
	}
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, texthtml, content_prog);
	Dir rt = fileSystem->openDir("/");
	while (rt.next())
	{
		String f = rt.fileName();
		if (f.startsWith("prog") && f.endsWith(extxt))
		{
			server.sendContent(F("loads('") + f.substring(5, f.length() - 4) + F("');\n"));
		}
	}
	server.sendContent(F("loadf('") + (server.args() > 0 ? server.arg(0) : "0") + F("');\n"));
	server.sendContent(F("</script>\n"));
	if (server.args() > 0) server.sendContent(F("saved ") + String(server.arg(1).length(), DEC) + F(" bytes<br>"));
	server.sendContent(F("</body>\n</html>\n"));
}

static const char content_progs[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<title>poi progs</title><meta charset='UTF-8'>
<style>input[type=number]{width:75px;}</style>
<script>
var onErr=function(err){console.warn(err);return new Response('error');}
function move(t,d){
	var row = t.parentNode.parentNode;
	var sib = d ? row.nextElementSibling : row.previousElementSibling;
	console.log(row, sib);
	if (d) {
		if (sib) row.parentNode.insertBefore(sib, row);
	} else {
		if (sib) row.parentNode.insertBefore(row, sib);
	}
}
async function save(n,b){
	var tb = b.nextElementSibling.nextElementSibling.rows;
	var s = '';
	for (var item of tb){
		s += item.children.item(1).textContent + ' ' + item.children.item(3).children.item(0).value + '\n';
	}
	var fd = new FormData();
	fd.append('prog', n);
	fd.append('vals', s);
	let r=await fetch('/progs',{method:'POST',body:fd}).catch(onErr);
	let t=await r.text();
	alert(t);
}
</script>
</head>
<body>
<a href='/'>main</a>   
<a href='/files'>files</a>   
<a href='/progs'>progs</a>   
<a href='/prog'>prog</a>   
<a href='/lis'>lis</a>
<br><br>
)=====";

void handleProgs()
{
	if (server.args() > 0)
	{
		String fname = F("/prog_") + server.arg(0).substring(0,20) + extxt;
		if (server.arg(0).startsWith("prog") && server.arg(0).endsWith(extxt))
		{
			fname = server.arg(0);
			if (fname.length() > 30) fname = fname.substring(0,25) + extxt;
		}
		String ans = F("saved file: ") + fname + F("\r\n");
		File prgfile = fileSystem->open(fname, "w");
		for (int i = 1; i < server.args(); i++)
		{
			ans += server.arg(i) + F("\r\n");
			prgfile.print(server.arg(i) + F(" ") + String(stat.bpm, DEC) + F("\n"));
		}
		prgfile.close();
		bmp_max();
		server.send(200, textplain, ans);
		return;
	}
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, texthtml, content_progs);
	Dir rt = fileSystem->openDir("/");
	while (rt.next())
	{
		String f = rt.fileName();
		if (f.startsWith("prog") && f.endsWith(extxt))
		{
			server.sendContent(f + F(" <input type='button' value='save' onclick=\"save('") + f + F("',this)\"><br>\n<table border=1>\n"));
			File prgfile = fileSystem->open(rt.fileName(), "r");
			String pic;
			while (prgfile.available())
			{
				pic = prgfile.readStringUntil('\n');
				int spidx = pic.lastIndexOf(" ");
				if (spidx == -1 || spidx == pic.length() - 1)
				{
					if (pic == "stop")
						server.sendContent(F("<tr><td></td><td>stop</td>\n"));
					else
						server.sendContent(F("<tr><td colspan=4>") + pic + F("<br>prog err: no second arg</td></tr>\n"));
				}
				else
				{
					String fpic = pic.substring(0, spidx);
					String dspic = pic.substring(spidx + 1);
					dspic.trim();
					server.sendContent(F("<tr>"
						"<td><input type=button value='&#9650;' onclick='move(this,0)'> "
							"<input type=button value='&#9660;' onclick='move(this,1)'></td>"
						"<td>") + fpic + F("</td><td><img src='") + fpic + F("'></td>"
						"<td><input type=number value='") + dspic + F("'></td></tr>\n"));
				}
			}
			prgfile.close();
			server.sendContent(F("</table><br>\n"));
		}
	}
	server.sendContent(F("</body>\n</html>\n"));
}

static const char content_conf1[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<title>poi config</title><meta charset='UTF-8'>
<style>
td {padding: 5px 0}
table, td:first-child {border: 1px solid #ccc}
table, td {min-width:30px}
</style>
</head>
<body>
<a href='/'>back</a><br><span id=ans></span><br>

<table><caption>-Configuration-</caption>

<tr><td align=right>Skip Wait for Wifi Connect: </td>
<td><label><input type=checkbox id=skwf> on</label></td>
<td><input type=button value=set onclick="r('skwf')"></td></tr>

<tr><td align=right>Skip Wifi Connect: </td>
<td><input  type=checkbox id=skpwc disabled></td>
<td></td></tr>

<tr><td align=right>use Lis3d: </td>
<td><label><input type=checkbox id=uselis> on</label></td>
<td><input type=button value=set onclick="r('uselis')"></td></tr>

<tr><td align=right>run by hit Lis3d: </td>
<td><label><input type=checkbox id=hitlis> on</label></td>
<td><input type=button value=set onclick="r('hitlis')"></td></tr>

<tr><td align=right>brightness Lis3d: </td>
<td><label><input type=checkbox id=brglis> on</label></td>
<td><input type=button value=set onclick="r('brglis')"></td></tr>

<tr><td align=right>speed Lis3d: </td>
<td><label><input type=checkbox id=spdlis> on</label></td>
<td><input type=button value=set onclick="r('spdlis')"></td></tr>

<tr><td align=right>start buttest: </td>
<td><input id=uptime size=7 disabled></td>
<td><input type=button value=go onclick="r('uptime')"></td></tr>

</table><br><br>

<table><caption>-Calibration-</caption>

<tr><td align=right>Vcc mv: </td>
<td><input type=number id=vcc min=100 max=5000></td>
<td><input type=button value=set onclick="r('vcc')"></td></tr>

<tr><td align=right>LedsCount: </td>
<td><input type=number id=leds min=1 max=255></td>
<td><input type=button value=set onclick="r('leds')"></td></tr>

<tr><td align=right>LedStrip Dir: </td>
<td><label><input type=checkbox id=dir> invert</label></td>
<td><input type=button value=set onclick="r('dir')"></td></tr>

<tr><td align=right>File delay micros <br>[0-255] x10: </td>
<td><input type=number id=fwait min=0 max=255></td>
<td><input type=button value=set onclick="r('fwait')"></td></tr>

<tr><td align=right>Add contrast (0-128): </td>
<td><input type=number id=cont min=0 max=128></td>
<td><input type=button value=set onclick="r('cont')"></td></tr>

<tr><td align=right>Old pcb Lis3d:</td>
<td><label><input type=checkbox id=oldlis> on</label></td>
<td><input type=button value=set onclick="r('oldlis')"></td></tr>

<tr><td align=right>Reset Calib: </td>
<td colspan=2 align=center>
	<input type=hidden id=facres value=1>
	<input type=button value=Reset onclick="if(confirm('Warning. Factory reset. Continue?')) r('facres')"></td></tr>

</table><br><br>

<script>
function r(p){
	var el=document.getElementById(p), v;
	if (el.type.toLowerCase()=='checkbox') v=el.checked?1:0;
	else {v=el.value; if(parseInt(v)<parseInt(el.min) || parseInt(v)>parseInt(el.max)) {alert('wrong value, use '+el.min+'-'+el.max);return;}}
	fetch('/req?'+p+'='+(v))
	.then((response) => {return response.text();})
	.then((data) => {document.getElementById('ans').innerHTML=data;});
}
function load(v){
	for(var key in v){
		var el=document.getElementById(key);
		if(el.type.toLowerCase()=='checkbox') el.checked=v[key]?true:false;
		else el.value=v[key];
	}
}
)=====";

static const char content_conf2[] PROGMEM = R"=====(
<form>
<table>
<caption>-EEPROM-</caption>
<tr><td align=right>key: </td><td><input name=k size=3></td></tr>
<tr><td align=right>value: </td>
<td><input name=v size=3><input type=submit value=set></td></tr>
</table>
</form>
<br>

<form onsubmit="return confirm('Warning. Eeprom clearing. Continue?');">
<input type=submit name=ce value='clear eeprom'>
</form><br><br>

</body></html>
)=====";

void handleConfig()
{
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, texthtml, content_conf1);
	String ans = F("load({skwf:"); ans += String(conf.skpwf);
	ans += F(",skpwc:"); ans += String(conf.skpwc);
	ans += F(",uselis:"); ans += String(conf.lis_on);
	ans += F(",hitlis:"); ans += String(conf.lis_hit);
	ans += F(", brglis:"); ans += String(conf.lis_brgn);
	ans += F(", spdlis:"); ans += String(conf.lis_spd);
	ans += F(", vcc:"); ans += String(getvcc(), DEC);
	ans += F(", leds:"); ans += String(conf.leds, DEC);
	ans += F(", dir:"); ans += String(conf.dir);
	ans += F(", fwait:"); ans += String(conf.fwait, DEC);
	ans += F(", cont:"); ans += String(conf.cont, DEC);
	ans += F(", oldlis:"); ans += String(conf.lis_oldpcb);
	ans += F(", uptime:"); ans += String(stat.uptime, DEC);
	ans += F("});\n"
		"</script>\n\n");
	server.sendContent(ans);
	if (server.args() > 0) {
		String san = server.argName(0);
		if (server.args() == 2 && san == "k")
		{
			int key = server.arg(0).toInt();
			int val = server.arg(1).toInt();
			if (server.arg(0).length() && server.arg(1).length())
			{
				EEPROM.write(key, val);
				server.sendContent(F("writed key ") + String(key, DEC) + F(" val ") + String(val, DEC));
			}
			else
			{
				server.sendContent(F("bad values"));
			}
		}
		if (server.args() == 1 && san == "ce")
		{
			for (int i = 0; i < EEP_SIZE; i++) EEPROM.write(i, 255);
			server.sendContent(F("eeprom cleared"));
		}
		EEPROM.commit();
	}
	server.sendContent(F("<table border=1><tr>"));
	for (int i = 0; i < EEP_SIZE; i++)
	{
		if (i % 32 == 0) server.sendContent(F("<td>") + String(i,DEC) + F("</td>"));
		char c = EEPROM.read(i);
		if (i < 32)
		{
			server.sendContent(F("<td>") + String(c, DEC) + F("</td>"));	
		}
		else if (i < 224)
		{
			if ((c > 31 && c < 60) || (c == 61) || (c > 62 && c < 127)) 
				server.sendContent(F("<td>") + String(c, DEC) + F("<br>") + String(c) + F("</td>"));
			else
				server.sendContent(F("<td>") + String(c, DEC) + F("</td>"));
		}
		else
		{
			server.sendContent(F("<td>") + String(c, HEX) + F("</td>"));
		}
		if ((i + 1) % 32 == 0) server.sendContent(F("</tr><tr>"));
	}
	server.sendContent(F("</tr></table>"));
	server.sendContent(content_conf2);
}

File fsUploadFile;

void handleFileUpload()
{
	HTTPUpload & upload = server.upload();
	stat.fcom = false;
	static bool odd = false;
	if (upload.status == UPLOAD_FILE_START)
	{
		String filename = upload.filename;
		int dot = filename.lastIndexOf(".");
		String name =  filename.substring(0, dot);
		String ext = filename.substring(dot);
		ext.toLowerCase();
		filename = name.substring(0,25) + ext;
		filename.replace("&","_");
		if (!filename.startsWith("/"))
		{
			filename = "/" + filename;
		}
		fsUploadFile = fileSystem->open(filename, "w");
		filename = String();
		led_clear();
		led_show();
	}
	else if (upload.status == UPLOAD_FILE_WRITE)
	{
		if (fsUploadFile)
		{
			fsUploadFile.write(upload.buf, upload.currentSize);
			odd = !odd;
			led_setpx(1, 0, 0, odd ? 255 : 0);
			led_setpx(2, 0, 0, odd ? 0 : 255);
			led_show();
		}
	}
	else if (upload.status == UPLOAD_FILE_END)
	{
		if (fsUploadFile)
		{
			fsUploadFile.close();
		}
		Serial.print(F("handleFileUpload Size: "));
		Serial.println(upload.totalSize);
		led_setpx(1, 0, 255, 0);
		led_setpx(2, 0, 0, 0);
		led_show();
		stat.calcmax = true;
		String upmsg = F("Upload ");
		upmsg += upload.filename;
		upmsg += F(" ok<script>setTimeout(function(){location.href='/files';},1000);</script>");
		server.send(200, texthtml, upmsg);
	}
}

void handleFileDelete()
{
	if (server.args() == 0)
	{
		return server.send(500, textplain, F("BAD ARGS"));
	}
	String path = server.arg(0);
	Serial.print(F("handleFileDelete: "));
	Serial.println(path);
	if (path == "/")
	{
		return server.send(500, textplain, F("BAD PATH"));
	}
	if (!fileSystem->exists(path)) {
		return server.send(404, textplain, F("FileNotFound"));
	}
	if(bmp_check(path))
	{
		fileSystem->remove(path);
		String newname = path.substring(0, path.length() - 3);
		if (fileSystem->exists(newname + "bma")) 
		{
			fileSystem->remove(newname + "bma");
			server.send(200, texthtml, F("ok, bma deleted"));
		}
		else
		{
			server.send(200, texthtml, F("ok"));
		}
		stat.calcmax = true;
	}
	else
	{
		fileSystem->remove(path);
		if (path.startsWith("prog") && path.endsWith(extxt)) bmp_max();
		server.send(200, texthtml, F("<a href='/files'>back</a><br>ok"));
	}
	path = String();
}

void ftp_calb(FtpOperation ftpOperation, unsigned int freeSpace, unsigned int totalSpace){
	if (ftpOperation == FTP_FREE_SPACE_CHANGE) stat.calcmax = true;
}

void http_begin()
{
	server.on(F("/"), handleRoot);
	server.on(F("/files"), handleFiles);
	server.on(F("/filesap"), handleFilesap);
	server.on(F("/fs.bin"), handleFsRead);
	server.on(F("/pics"), handlePics);
	server.on(F("/dec"), handleDec);
	server.on(F("/req"), handleReq);
	server.on(F("/lis"), handleLis);
	server.on(F("/prog"), handleProg);
	server.on(F("/progs"), handleProgs);
	server.on("/prog.js", []() {
		server.setContentLength(prog_js_gz_len);
		server.sendHeader("Content-Encoding", "gzip");
		server.send(200, "application/javascript", "");
		server.sendContent(prog_js_gz, prog_js_gz_len);
	});
	server.on(F("/config"), handleConfig);
	server.on(F("/load"), HTTP_POST, []() {
		server.send(200, textplain, "");
	}, handleFileUpload);
	server.on(F("/del"), handleFileDelete);
	server.on(F("/view.svg"), HTTP_GET, []() {server.send(200, F("image/svg+xml"), view_svg);});
	server.on(F("/noview.svg"), HTTP_GET, []() {server.send(200, F("image/svg+xml"), noview_svg);});
	httpUpdater.setup(&server);
	server.onNotFound(handleNotFound);
	server.begin();
	Serial.println(F("HTTP server started"));
	ftpSrv.setCallback(ftp_calb);
	ftpSrv.begin("Welcome!");
	Serial.println(F("FTP server started"));
}

void http_beginap()
{
	httpUpdater.setup(&server);
	server.onNotFound([]() {
		server.send(200, texthtml, F("<a href='/update'>update</a>"));
	});
	server.begin();
}

void http_poll()
{
	server.handleClient();
}

void ftp_poll()
{
	ftpSrv.handleFTP();
}

