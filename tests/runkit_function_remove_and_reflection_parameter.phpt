--TEST--
runkit_function_remove() function with ReflectionParameter
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
function runkitFunction($param) {
	echo "Runkit Function\n";
}

$reflFunc = new ReflectionFunction('runkitFunction');
$reflParam = $reflFunc->getParameters();
$reflParam = $reflParam[0];

runkit_function_remove('runkitFunction');

var_dump($reflParam);
try {
	var_dump($reflParam->getDeclaringFunction());
} catch (Error $e) {
	echo "\n";
	printf("(No longer a )Fatal error: %s in %s on line %d", $e->getMessage(), $e->getFile(), $e->getLine());
}
?>
--EXPECTF--
object(ReflectionParameter)#%d (1) {
  ["name"]=>
  string(31) "__parameter_removed_by_runkit__"
}

%sFatal error:%sInternal error: Failed to retrieve the reflection object in %s on line %d
