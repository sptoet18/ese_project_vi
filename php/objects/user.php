<?php

namespace objects;

enum Role: string {
    case admin = 'admin';
    case projectManager = 'project_member';
    case projectSupervisor = 'project_supervisor';
    case eseStudent = 'ese_student';
    case user = 'user';
}
class user
{
    private int $id;
    private string $username;
    private string $hashedPassword;
    private string $firstname;
    private string $lastname;
    private Role $role;
    private \DateTimeImmutable $createdAt;
    private string $archived;

    public function __construct(string $username, string $password, string $firstname, string $lastname, Role $role) {
        $this->username = $username;
        $this->hashedPassword = password_hash($password, PASSWORD_DEFAULT);
        $this->firstname = $firstname;
        $this->lastname = $lastname;
        $this->role = $role;
    }
    public function create() {
        $db = connect();

        try {
            $query = '
                INSERT INTO user(
                                 username, 
                                 hashed_password, 
                                 firstname, 
                                 lastname, 
                                 role
                ) VALUES (
                          :username, 
                          :hashed_password, 
                          :firstname, 
                          :lastname, 
                          :role
                );
    
                SELECT LAST_INSERT_ID() AS id;
            ';
            $statement = $db->prepare($query);
            $statement->execute([
                'username' => $this->username,
                'hashed_password' => $this->hashedPassword,
                'firstname' => $this->firstname,
                'lastname' => $this->lastname,
                'role' => $this->role
            ]);

            $id = $db->lastInsertId();
        } catch (\PDOException $e) {
            die("Creation failed: " . $e->getMessage());
        }

        return $id;
    }

    public function get(int $id) : user {
        $db = connect();

        try {
            $query = '
                SELECT *
                FROM user
                WHERE id = :id
                LIMIT 1
            ';
            $statement = $db->prepare($query);
            $statement->execute([
                'id' => $id
            ]);
            $currentUser = $statement->fetch();
        } catch (\PDOException $e) {
            die("Get failed: " . $e->getMessage());
        }

        return $currentUser;
    }
}