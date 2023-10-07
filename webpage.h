const char * html_page = R""""(
<html>
<head>
<style>
.button {
  display: inline-block;
  padding: 10px 25px;
  cursor: pointer;
  text-align: center;
  text-decoration: none;
  outline: none;
  color: #fff;
  width: 150px;
  background-color: #00aaff;
  border: none;
  border-radius: 15px;
  box-shadow: 0 3px #999;
}
.button2 {
	background-color:#00bb00;
}
.button:active {
  background-color: #22222;
  box-shadow: 0 3px #666;
  transform: translateY(3px);
}
</style>
</head>
<body onload='init()'>
<p><h1>ESP LED Driver</h1></p>
<p><h2 id='effect'></h2></p>
<table>
<tr>
	<th>Brightness:</th>
	<th><input id='brightness' type='range' onChange="sliderFunction('brightness',this.value)" min='1' max='255' value='255'></th>
	<th>R<input id='col_r' type='range' onChange="sliderFunction('r',this.value)" min='0' max='255' value='255'></th>
</tr><tr>
	<th>Fade Speed:</th>
	<th><input id='fade' type='range' onChange="sliderFunction('fade',this.value)" min='1' max='255' value='50'></th>
	<th>G<input id='col_g' type='range' onChange="sliderFunction('g',this.value)" min='0' max='255' value='0'></th>
</tr><tr>
	<th>Effect Speed:</th>
	<th><input id='speed' type='range' onChange="sliderFunction('speed',this.value)" min='1' max='255' value='10'></th>
	<th>B<input id='col_b' type='range' onChange="sliderFunction('b',this.value)" min='0' max='255' value='0'></th>
</tr><tr>
	<th>Color Change:</th>
	<th><input id='hue' type='range' onChange="sliderFunction('hue',this.value)" min='1' max='255' value='10'></th>
</tr><tr>
	<th>Fixed Color<input type='checkbox' onChange="sliderFunction('rgb',this.checked)" id='rgb'></th>
	<th>Endless<input type='checkbox' onChange="sliderFunction('endless',this.checked)" id='endless'></th>
	<th>Reverse<input type='checkbox' onChange="sliderFunction('reverse',this.checked)" id='reverse'></th>
</tr><tr>
<td colspan='3'>Effects with all Parameters</td>
</tr><tr>
	<th><input type='button' onclick='buttonFunction(1,this.value)' class='button' value='K.I.T.T'></th>
	<th><input type='button' onclick='buttonFunction(7,this.value)' class='button' value='Sinelon'></th>
	<th><input type='button' onclick='buttonFunction(5,this.value)' class='button' value='Constant'></th>
</tr><tr>
	<th><input type='button' onclick='buttonFunction(3,this.value)' class='button' value='Mirror'></th>
	<th><input type='button' onclick='buttonFunction(2,this.value)' class='button' value='Multiband'></th>
	<th><input type='button' onclick='buttonFunction(6,this.value)' class='button' value='Constant Fade'></th>
</tr><tr>
	<td colspan='3'>Effects supporting only Fade and Speed:</td>
</tr><tr>
	<th><input type='button' onclick='buttonFunction(16,this.value)' class='button' value='Juggle'><br></th>
	<th><input type='button' onclick='buttonFunction(15,this.value)' class='button' value='BPM'></th>
	<th><input type='button' onclick='buttonFunction(13,this.value)' class='button' value='Confetti'></th>
</tr><tr>
	<th><input type='button' onclick='buttonFunction(8,this.value)' class='button' value='Fire'></th>
	<th><input type='button' onclick='buttonFunction(12,this.value)' class='button' value='Rainbow with Glitter'></th>
</tr><tr>
	<td colspan='3'>Effect without Parameters:</td>
</tr><tr>
	<th><input type='button' onclick='buttonFunction(4,this.value)' class='button' value='Pride'></th>
	<th><input type='button' onclick='buttonFunction(10,this.value)' class='button' value='Pacifica'></th>
	<th><input type='button' onclick='buttonFunction(11,this.value)' class='button' value='Rainbow'></th>
</tr><tr>
<th></th>
</tr><tr>
	<th><input type='button' onclick='buttonFunction(100)' class='button button2' value='Save'><br></th>
	<th><input type='button' onclick='buttonFunction(0,this.value)' class='button button2' value='OFF'><br></th>
	<th><input type='button' onclick='buttonFunction(101,this.value)' class='button button2' value='Timer 1h'><br></th>
</tr>
<script>
  const effects=['OFF','K.I.T.T.','Multiband','Mirror','Pride','Constant','Constant Fade','Sinelon','Fire','Multi(fast)',
                'Pacifica','Rainbow','Rainbow mit Glitter','Confetti','Confetti (fast)','BPM','Juggle'];
  function init() {
	  updateValues();
	  var id=setInterval(updateValues,5000);
  }
  function updateValues() {
    const req = new XMLHttpRequest();
    req.open('GET','/settings');
    req.send();
    req.onreadystatechange = () => {
      if (req.readyState === XMLHttpRequest.DONE) {
        const status = req.status;
        if (status == 200) {
            var reply = JSON.parse(req.responseText);
            document.getElementById('rgb').checked=(reply.rgb==1?true:false);
            document.getElementById('endless').checked=(reply.endless==1?true:false);
            document.getElementById('reverse').checked=(reply.reverse==1?true:false);
            document.getElementById('speed').value= reply.speed;
            document.getElementById('fade').value= reply.fade;
            document.getElementById('brightness').value= reply.brightness;
            document.getElementById('col_r').value = reply.col_r;
            document.getElementById('col_g').value = reply.col_g;
            document.getElementById('col_b').value = reply.col_b;
            document.getElementById('hue').value = reply.hue;
            console.log(reply.effect);
			var thum="";
			if (reply.temperature) {
			   thum="<p>Temperature "+reply.temperature+" &deg;C</p>Humidity: "+reply.humidity+" %</p>";
			}
            document.getElementById('effect').innerHTML = "Current Effect: "+effects[reply.effect]+reply.timer+thum;
        }
      }
    };
  }
  function buttonFunction(id,value) {
      const req = new XMLHttpRequest();
      req.open('POST','/command?id='+id);
      req.send();
      updateValues();
      if (id<100) {
      document.getElementById('effect').innerHTML = 'Current Effect: '+effects[id]; }
    return false;
  }
  function sliderFunction(name,val) {
      console.log(name+'='+val);
      const req = new XMLHttpRequest();
      req.open('POST','/slider?'+name+'='+val);
      req.send();
      return false;
  }
</script>
</body>
</html>
)"""";

const char * login_page = R""""(
<html>
<head>
<style>
.button:active {
  transform: translateY(3px);
}
</style>
</head>
<body>
<p><h1>ESP LED Login Page</h1></p>
<p>SSID:<input type='text' size=25 id='ssid'></p>
<p>Password:<input type='text' size=25 id='pw'></p>
<p><input type='button' onclick='submit()' value='Submit'></p>
<script>
function submit() {
	const req = new XMLHttpRequest();
	var ssid=document.getElementById('ssid').value;
	var pw=document.getElementById('pw').value;
	var uri="ssid="+encodeURIComponent(ssid)+"&pw="+encodeURIComponent(pw);
    req.open('POST',"/login?"+uri);
    req.send();
    return false;
}
</script>
)"""";
