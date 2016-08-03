<?php
function tripal_galaxy_add_webform_tool_input(&$components, &$conditionals, $tool_input, $steps, $step_index, $pid = 0) {
	
	
	// We start out in the inputs of a given tool, we will either see test_param/cases tool (conditional) or some type of tool that is simply just a fill in the blank
	

	$options = array();
	$component = array ();
	$is_list = 0;
	$webform_value = "";
	$webform_type = "";
	$input_name = '';
	$is_required = 0;
	$extra = array(
			'title_display' => 'before',
			'description_above' => 1,
			'private' => 0,
			'items' => "",
			'aslist' => "",
			'multiple' => 0,
	);

	// Is this field required?
	if (array_key_exists('optional', $tool_input) and $tool_input['optional']) {
		$is_required = 1;
	}
	
	// Is this a private field?
	if (array_key_exists('hidden', $tool_input) and $tool_input['hidden']) {
		$extra['private'] = 1;
	}
	
	if (array_key_exists('multiple', $tool_input) and $tool_input['multiple']) {
		$extra['multiple'] = 1;
	}
	
	if (array_key_exists('help', $tool_input) and $tool_input['help']) {
		$extra['description'] = $tool_input['help'];
	}
	
	
	// Extract tool input form information to a drupal webform component.
	switch ($tool_input['type']) {
		case 'text':
			// Text should be the webform equivalent of textfield.
			$webform_type = 'textfield';
			if(array_key_exists('value', $tool_input)){
				$webform_value = $tool_input['value'];
			}
			break;
	
		case 'select':
			// Select should be the default list drop-down. Unless it has a display
			// of Checkbox
			if ($tool_input['display'] == "checkboxes") {
				$extra['aslist'] = 0;
				$extra['multiple'] = 1;
			}
			break;
	
		case 'data_column':
			// Data_column should be the default list drop-down.
			$extra['multiple'] = 0;
			break;
	
		case 'genomebuild':
			break;
	
		case 'data':
			// Data appears to be a file selection of files that already
			// exists in the user's history.
			$webform_type = 'select';
			foreach ($tool_input['options'] as $key => $vals) {
				foreach ($vals as $details) {
					$extra['items'] .= $details['id'] . '|' . $details['name'] . "\n";
				}
			}
			$webform_value = '';
			break;
	
		case 'data_collection':
			break;
	
		case 'integer':
			$webform_type = 'number';
			break;
	
		case 'float':
			break;
	
		case 'boolean':
			// Boolean should have a single true/false value. It is also likely to
			// not have 'options' So we must invent our own
			$extra['items'] = "t|True\nf|False";
			$extra['aslist'] = 0;
			break;
	
		case 'drill_down':
			break;
	
		case 'color':
			break;
	
		case 'hidden':
			break;
	
		case 'hidden_data':
			break;
	
		case 'baseurl':
			break;
	
		case 'library_data':
			break;
	
		case 'ftpfile':
			break;
	
		case 'repeat':
			break;
	
		case 'conditional':
			
			
			// Here a loop needs to be intsantiated
			
			
			
			
			
			// A conditional case contains nested components but those components
			// should only be shown when specific selections are made in other
			// components. We will recursively add all the components and then
			// create the conditional rules that webforms will use to show the
			// conditional fields.
	
			// First add the test param as this is used for the conditional
			tripal_galaxy_add_webform_tool_input($components, $conditionals, $tool_input['test_param'], $steps, $step_index, $pid);
			$num_components = count($components) - 1;
			$source_id = $components[$num_components]['cid'];
	
			// Add the case paramters.
			foreach ($tool_input['cases'] as $case) {
				$conditional = array(
						'andor' => NULL,
						'weight' => count($conditionals),
						'rules' => array(
								array(
										'source_type' => 'component',
										'source' => $source_id,
										// element will be listening
										'operator' => 'equal',
										// This is the option for which this conditional applies.
										'value' => $case['value'],
								),
						),
						'actions' => array(),
				);
	
				// Iterate through the cases and add a new component and a new
				// conditional action.
				foreach ($case['inputs'] as $cindex => $input) {
	
					tripal_galaxy_add_webform_tool_input($components, $conditionals, $input, $steps, $step_index, $pid);
					$num_components = count($components) - 1;
					$target_id = $components[$num_components]['cid'];
	
					$conditional['actions'][] = array(
							'target_type' => 'component',
							'target' => $target_id,
							'action' => 'show',
							'argument' => '',
					);
				}
	
				$conditionals[] = $conditional;
			}
	
			// We won't add a 'conditional' as a component so we can short circuit
			// this function and return.
			return;
			break;
		default :
			drupal_set_message('Unhandled tool input type: ' . $tool_input['type'], 'error');
			break;
	}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	

// Initiate variables. By default, these variables will create
// A drop-down list webform component.
if(array_key_exists('options', $tool_input)) {
	$items = '';
	// Generate the options that will be put under
	foreach ($tool_input['options'] as $key => $val) {
		// The val is now that sub-array containing the human readable option
		// (index 0) as well as the unique key-name (index 1).
		if(is_array($val) and !empty($val) and count($val) >= 2) {
			$items .= $val[1] . '|' . $val[0] . "\r\n";
		}
	}
}

// If there are no options for the tool then set the items to the
// default value if one is provided.
if(empty($items) and array_key_exists('text_value', $tool_input)) {
	$items = $tool_input['text_value'];
}


$options = array();
$component = array ();
$is_list = 1;
$webform_value = "";
$webform_type = 'select';
$input_name = '';
$is_required = 0;
$extra = array(
		'title_display' => 'before',
		'description_above' => 1,
		'private' => 0,
		'items' => $items,
		'aslist' => $is_list,
		'multiple' => 0,
);

// Set the default value accordingly.
if(array_key_exists('value', $tool_input)){
	$webform_value = $tool_input['value'];
}

// Find the options.
if (array_key_exists('options', $tool_input)) {
	$options = $tool_input['options'];
}




// Get the input name.
$input_name = NULL;
if (array_key_exists('label', $tool_input)) {
	$input_name = $tool_input['label'];
}
else if(array_key_exists('title', $tool_input)) {
	$input_name = $tool_input['title'];
}


// If this field recieves it's value from a previous step then we want
// to set the type as markup and print that information
$tool_name = $tool_input['name'];
if (array_key_exists($tool_name, $steps[$step_index-1]['input_steps'])){
	$source_step = $steps[$step_index-1]['input_steps'][$tool_name]['source_step'];
	$step_output = $steps[$step_index-1]['input_steps'][$tool_name]['step_output'];
	$webform_type = 'markup';
	$extra['format'] = 'full_html';
	$webform_value =  "<label>" . $input_name . "</label>" .
			"Output from step " . ($source_step + 1);
}


// Default component. Defaults to a drop-down list component.
$cid = count($components) + 1;
$components[] = array(
		'cid' => $cid,
		'pid' => $pid,
		'name' => $input_name,
		'form_key' => "Step_" . $step_index ."_" . $cid,
		'type' => $webform_type,
		'value' => $webform_value,
		'extra' => $extra,
		'weight' => $cid,
		'required' => $is_required,
);
}