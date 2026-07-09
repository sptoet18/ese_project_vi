<?php
    session_start();

    if (isset($_SESSION['username'])) {
        // this is for members only
    } else {
        // todo: redirect to login page
    }
?>