Auf der Branch "main" sollten nur die jeweiligen Übungen ohne Änderungen sein. 
Fertigen Bearbeitungen zur Abgabe, entweder in eine "Abgabe" Branch und dann per PullRequest oder nach absprache/fixes auf main.

Alle anderen Änderungen oder Bearbeitungen bitte auf den jeweis eigenen Branches "UserName".
Zur Besseren Übersicht wäre es hilfreich, wenn z.B. Blatt1 - Aufgabe 1 (etc.) als eigenständiger Commit auf der Branch ist.
Solltet ihr über die CMD arbeiten, hier ein paar Befehle die helfen könnten:


Erstellen der eigenen Branch (copy of main):

git checkout main;
git pull;
git checkout -b "NewBranchName";
git push --set-upstream origin "NewBranchName";


NUR NEUE Dateien von main in die eigene Branch:

git checkout main;
git pull;
git checkout "BranchName";
git pull;
git rebase main -X theirs;
