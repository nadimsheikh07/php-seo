<?php

$userAgent = $_SERVER['HTTP_USER_AGENT'];
if (strpos($userAgent, 'bot') !== false && strpos($userAgent, 'http') !== false) {
    // It is probably a bot
    echo 'BOT';
} else {
    echo 'USER';
}
