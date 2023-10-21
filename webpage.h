const char * html_page = R""""(
<html>
<head>
<style>
body {
    font-family: Arial, sans-serif;
    background-color: #f0f0f0;
    text-align: center;
  }
.container {
    max-width: 800px;
    margin: 0 auto;
    padding: 10px;
    background-color: #fff;
    border-radius: 15px;
    box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
  }
.button {
  display: inline-block;
  padding: 10px 25px;
  cursor: pointer;
  text-align: center;
  text-decoration: none;
  outline: none;
  color: #fff;
  width: 100%;
  background-color: #00aaff;
  border: none;
  border-radius: 15px;
  box-shadow: 0 3px #999;
  font-size: 24px;
}
.button2 {
	background-color:#00bb00;
}
.button:active {
  background-color: #22222;
  box-shadow: 0 3px #666;
  transform: translateY(3px);
}
input[type="checkbox"] {
  vertical-align: middle;
  height: 30px;
  width: 30px;
}
input[type="color"] {
  vertical-align: middle;
  width: 100%;
  height: 50px;
}
table {
  width: 100%;
  font-size: 24px;
  background-color: #eee;
  box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
  border-radius: 15px;
}
th {
  color: #000;
  padding: 2px;
}
td {
  padding: 5px;
}
input[type="range"] {
  width: 100%;
}
</style>
</head>
<body onload='init()'>
<div class="container">
<p><h1>ESP LED Driver</h1></p>
<p><h2 id='effect'></h2></p>
<table>
<tr>
	<th>Brightness</th><th>Fade Speed</th><th>Effect Speed</th>
</tr><tr>
	<th><input id='brightness' type='range' onChange="sliderFunction('brightness',this.value)" min='1' max='255' value='255'></th>
	<th><input id='fade' type='range' onChange="sliderFunction('fade',this.value)" min='1' max='255' value='50'></th>
	<th><input id='speed' type='range' onChange="sliderFunction('speed',this.value)" min='1' max='255' value='10'></th>
</tr><tr>
</tr><tr>
	<th>Color Change:</th>
	<th><input id='hue' type='range' onChange="sliderFunction('hue',this.value)" min='1' max='255' value='10'></th>
  <th><input type='color' size=3 onChange="sliderFunction('color',this.value)" id='color'></th>
</tr><tr>
 	<th><input type='checkbox' onChange="sliderFunction('rgb',this.checked)" id='rgb'> Color Rotation </th>
	<th><input type='checkbox' onChange="sliderFunction('endless',this.checked)" id='endless'> Endless</th>
	<th><input type='checkbox' onChange="sliderFunction('reverse',this.checked)" id='reverse'> Reverse</th>
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
	<th><input type='button' onclick='buttonFunction(12,this.value)' class='button' value='Rainbow+Glitter'></th>
  <th><input type='button' onclick='buttonFunction(14,this.value)' class='button' value='Confetti (fast)'></th>
</tr><tr>
	<td colspan='3'>Effect without Parameters:</td>
</tr><tr>
	<th><input type='button' onclick='buttonFunction(4,this.value)' class='button' value='Pride'></th>
	<th><input type='button' onclick='buttonFunction(10,this.value)' class='button' value='Pacifica'></th>
	<th><input type='button' onclick='buttonFunction(11,this.value)' class='button' value='Rainbow'></th>
</tr><tr>
	<td colspan='3'></td>
</tr><tr>
	<th><input type='button' onclick='buttonFunction(100)' class='button button2' value='Save'><br></th>
	<th><input type='button' onclick='buttonFunction(0,this.value)' class='button button2' value='OFF'><br></th>
	<th><input type='button' onclick='buttonFunction(101,this.value)' class='button button2' value='Timer 1h'><br></th>
</tr>
</div>
<script>
  const effects=['OFF','K.I.T.T.','Multiband','Mirror','Pride','Constant','Constant Fade','Sinelon','Fire','Multi(fast)',
                'Pacifica','Rainbow','Rainbow+Glitter','Confetti','Confetti (fast)','BPM','Juggle'];
  function init() {
	  updateValues();
	  var id=setInterval(updateValues,10000);
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
            document.getElementById('color').value= reply.color;
            console.log(reply.color);
            document.getElementById('speed').value= reply.speed;
            document.getElementById('fade').value= reply.fade;
            document.getElementById('brightness').value= reply.brightness;
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
      req.open('POST','/slider?'+name+'='+encodeURIComponent(val));
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
