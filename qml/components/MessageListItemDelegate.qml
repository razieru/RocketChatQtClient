import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    required property string text
    required property string author
    required property double timestampTicks
    readonly property var messageDate: timestampTicks > 0 ? new Date(timestampTicks * 1000) : null
    readonly property var nowDate: new Date()
    readonly property bool isToday: messageDate !== null
        && messageDate.getFullYear() === nowDate.getFullYear()
        && messageDate.getMonth() === nowDate.getMonth()
        && messageDate.getDate() === nowDate.getDate()
    readonly property string formattedTimestamp: timestampTicks > 0
        ? (isToday
            ? Qt.formatDateTime(messageDate, "HH:mm")
            : Qt.formatDateTime(messageDate, "dd.MM.yyyy HH:mm:ss"))
        : ""

    color: "transparent"
    border.color: "#333333"
    border.width: 1
    radius: 6

    implicitHeight: msgColumn.implicitHeight + 12

    ColumnLayout {
        id: msgColumn
        anchors.fill: parent
        anchors.margins: 6
        spacing: 2

        Label {
            text: (root.author || "unknown") +
                  (root.formattedTimestamp ? ("  [" + root.formattedTimestamp + "]") : "")
            color: "#9aa4b2"
            font.pixelSize: 11
            Layout.fillWidth: true
            elide: Label.ElideRight
        }

        Label {
            text: root.text || "(empty message)"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
