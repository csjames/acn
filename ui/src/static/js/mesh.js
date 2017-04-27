function getRandomColor() {
    var letters = '0123456789ABCDEF';
    var color = '#';
    for (var i = 0; i < 6; i++ ) {
        color += letters[Math.floor(Math.random() * 16)];
    }
    return color;
}

$('#sendColor').on('click', function() {
	if (r == true) {
		var color = $("#color").val()
	    
	}
});

$('#broadcastColor').on('click', function() {
	if (r == true) {
		var color = $("#color").val()
	}
});

$('#clearNodes').on('click', function () {
	var r = confirm("Confirm clear nodes");
	if (r == true) {
	    $.get('/clearNodes', function(data, status){
	    	location.reload();
	    });
	}
});

$('#clearMessages').on('click', function() {
	var r = confirm("Confirm clear messages");
	if (r == true) {
	    $.get('/clearMessages', function(data, status) {
	    	location.reload();
	    });
	}
});

var nodes =[];
var edges =[];
var network;
var state;

function prependMessage(message) {
	var direction = String.fromCharCode(message.Direction);
	console.log("Direction: " + direction);
	switch(direction) {
		case 'i':
			direction = "incoming";
			break
		case 'r':
			direction = "repeated";
			break
		case 'o':
			direction = "outbound";
			break
	}

	var s = String.fromCharCode(message.Type) + " " + direction + " To: " + message.Destination + " From: " + message.Sender + " Origin: " + message.Origin;
	$('#stream').prepend(s + "<br>");
}

// darkness kind of stays. need to sort that out.
function addMessageStyle(message) {
	prependMessage(message)
	console.log("e"+message.Sender+message.Destination)
	gray = (239-24*state.Messages.indexOf(message)).toString(16);

	var color;
	var type = String.fromCharCode(message.Type);
	console.log("TYPE: " + type);
	color = "#" + gray + gray + gray;
	console.log("Color : " + color)
	network.$("#e" + message.Sender + message.Destination).style("line-color", color);
}

$(function(){ // on dom ready

	$('#nodeids').empty();

	$.get('/current', function(data, status) {
		data = JSON.parse(data)
		state = data;
	    console.log(JSON.stringify(data));

	    data.Nodes.forEach(function (node) {
	    	console.log(node)
	    	nodes.push({data: {id: node}});
		    $('#nodeids').append($('<option></option>').val(node).html(node));
		});

	    data.Messages.forEach(function (message) {
	    	console.log("e"+message.Sender+message.Destination)
	    	edges.push({data: { id: "e"+message.Sender+message.Destination,source: message.Sender, target: message.Destination }});
	    });

		network = cytoscape({
		  container: document.getElementById('mesh'),
		  style: [
		    {
		      selector: 'node',
		      css: {
		        'content': 'data(id)',
		        'text-valign': 'center',
		        'text-halign': 'center',
		      }
		    },
		    {
		    	selector: 'edge',
		    	css: {
		    		'curve-style': 'bezier',
 					'target-arrow-shape': 'triangle',
		    	}
		    },
		    {
	        selector: ':selected',
	        style: {
	          'border-width': 1,
	          'border-style': 'solid',
	          'border-color': 'black',
	          'background-color': ''
	        }
	      	}
		  ],
		  elements: {
		    nodes: nodes,
		    edges: edges,
		  },

		  layout: {
		    name: 'breadthfirst',
		    padding: 5
		  }
		});

		network.$("#13").style("width","50px");
		network.$("#13").style("height","50px");

		data.Messages.forEach(function (message) {
			addMessageStyle(message);
	    });
	});

	var socket = new WebSocket('ws://' + window.location.host + "/sub");

	socket.onopen = function(event) {
	  console.log("Connected to server.")
	};

	// Handle messages sent by the server.
	socket.onmessage = function(event) {
	  console.log("received.")
	  var data = event.data;
	  var message = JSON.parse(data);

	  network.add({ group: "nodes" , data: {id: message.Sender}});
	  network.add({ group: "nodes" , data: {id: message.Destination}});
	  if (message.Origin !== "0") {
	  	network.add({ group: "nodes" , data: {id: message.Origin}});
	  }

	  prependMessage(message);

	  $.get('/current', function(data, status) {
		data = JSON.parse(data)
		state = data;
	    console.log(JSON.stringify(data));

	    data.Messages.forEach(function (message) {
	    	console.log("e"+message.Sender+message.Destination)
	    	edges.push({data: { id: "e"+message.Sender+message.Destination,source: message.Sender, target: message.Destination }});
	    });

		network.$("#13").style("width","50px");
		network.$("#13").style("height","50px");

		network.layout();

		state.Messages.forEach(function (message) {
			addMessageStyle(message)
	    });
	});
	};
});
