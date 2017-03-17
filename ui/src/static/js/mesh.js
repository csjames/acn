function getRandomColor() {
    var letters = '0123456789ABCDEF';
    var color = '#';
    for (var i = 0; i < 6; i++ ) {
        color += letters[Math.floor(Math.random() * 16)];
    }
    return color;
}

$('#sendColor').on('click', function() {
	var r = confirm("Confirm clear messages");
	if (r == true) {
		var color = $("#color").val()
	    
	}
});

$('#broadcastColor').on('click', function() {
	var r = confirm("Confirm clear messages");
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

		network.$("#1").style("width","70px");
		network.$("#1").style("height","70px");


		data.Messages.forEach(function (message) {
	    	console.log("e"+message.Sender+message.Destination)
	    	gray = (239-16*state.Messages.indexOf(message)).toString(16);
	    	color = "#" + gray + gray + gray
	    	console.log(color)
	    	network.$("#e" + message.Sender + message.Destination).style("line-color", color);
	    });
	});

		// Create a client instance
	client = new Paho.MQTT.Client("127.0.0.1", 9900, "clientId");

	// set callback handlers
	client.onConnectionLost = onConnectionLost;
	client.onMessageArrived = onMessageArrived;

	// connect the client
	client.connect({onSuccess:onConnect});


	// called when the client connects
	function onConnect() {
	  // Once a connection has been made, make a subscription and send a message.
	  console.log("onConnect");
	  client.subscribe("World");
	  message = new Paho.MQTT.Message("Hello");
	  message.destinationName = "World";
	  client.send(message);
	}

	// called when the client loses its connection
	function onConnectionLost(responseObject) {
	  if (responseObject.errorCode !== 0) {
	    console.log("onConnectionLost:"+responseObject.errorMessage);
	  }
	}

	// called when a message arrives
	function onMessageArrived(message) {
	  console.log("onMessageArrived:"+message.payloadString);
	}
});
