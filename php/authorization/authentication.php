<?php
    session_start();

    $username = $_POST['username'];
    $password = $_POST['password'];

    if ($username && $password) {
        $_SESSION['username'] = $username;

        // todo: redirect to members page
    } else {
        // todo: redirect to login page
    }
?>