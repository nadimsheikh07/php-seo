<?php

/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
// load the SimpleCloak library
require_once 'include/simple_cloak.inc';
// Use cloaking to render text instead of images
if (SimpleCloak::isSpider()) {
    echo 'BOT';
} else {
    echo 'USER';
}
