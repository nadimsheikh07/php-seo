<?php

// load the SimpleCloak library
require_once 'include/simple_cloak.inc';
// update cloaking data and indicate the success status
if (SimpleCloak::updateAll()) {
    echo "Cloaking database updated!";
} else {
    echo "Cloaking database was already up to date, or the update failed.";
}
