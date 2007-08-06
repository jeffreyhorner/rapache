function RePlot(){
	var p1 = document.spower.p1.value;
	var p2 = document.spower.p2.value;

	new  Ajax.Updater( 'plot', '/brew/useR2007plot.rhtml',
		{
			'method': 'get', 
			'parameters': {'p1': p1, 'p2': p2},
		}
	);
}
function ReSimulate(){
	var p1 = document.spower.p1.value;
	var p2 = document.spower.p2.value;

	Element.show('spinner');
	new  Ajax.Updater( 'spowerResult', '/brew/useR2007sim.rhtml',
		{
			'method': 'get', 
			'parameters': {'p1': p1, 'p2': p2},
			'onSuccess': function(r){Element.hide('spinner')}
		}
	);
}
