function getRandomColor() {
    var letters = '0123456789ABCDEF';
    var color = '#';
    for (var i = 0; i < 6; i++ ) {
        color += letters[Math.floor(Math.random() * 16)];
    }
    return color;
}

$(function(){ // on dom ready
	var network = cytoscape({
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
	    nodes: [
	      { data: { id: 'Gateway' } },
	      { data: { id: '2' } },
	      { data: { id: '3' } },
	      { data: { id: '4' } },
	      { data: { id: '5' } },
	      { data: { id: '6' } },
	      { data: { id: '7' } },
	      { data: { id: '8' } }
	    ],
	    edges: [
	      { data: { source: 'Gateway', target: '2' } },
	      { data: { source: '2', target: 'Gateway' } },
	      { data: { source: '3', target: 'Gateway' } },
	      { data: { source: '8', target: '2' } },
	      { data: { source: '6', target: '5' } },
          { data: { source: '4', target: 'Gateway' } },
	      { data: { source: '5', target: 'Gateway' } },
	      { data: { source: '7', target: '4' } },
	      { data: { source: '8', target: '3' } },
	    ]
	  },
	  
	  layout: {
	    name: 'breadthfirst',
	    padding: 5
	  }
	});

	network.$("#Gateway").style("width","70px");
	network.$("#Gateway").style("height","70px");
	// network.layout();
});