<?php
    error_reporting(E_ALL);
    ini_set('display_errors', 1);
    ini_set('display_startup_errors', 1);

    require_once __DIR__ . '/../objects/User.php';

    use objects\User;
    use objects\Role;

    session_start();

	$username = $_POST['username'];
    $password = $_POST['password'];
    $firstname = $_POST['firstname'];
    $lastname = $_POST['lastname'];
    $role = $_POST['role'];

    if (Role::tryFrom($role) === null) {
        header("location: ../../html/authorization/signup.html");
    }

    User::create($username, $password, $firstname, $lastname, $role);

    $_SESSION['username'] = $username;
    header('Location: member.php');