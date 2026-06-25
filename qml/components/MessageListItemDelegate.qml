import QtQuick
//import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Basic

Item {
    id: root
    required property string text
    required property string author
    required property double timestampTicks
    required property string quotePreviewText
    required property string quotedMessageId
    property var usersModel: null
    property bool hideUsernames: false
    property string currentUsername: ""
    signal quoteClicked(string messageId)
    height: implicitHeight
    readonly property bool isWide: width > 700
    readonly property bool isMineMessage:
        author.length > 0
        && currentUsername.length > 0
        && author.toLowerCase() === currentUsername.toLowerCase()
    readonly property real messageMaxWidth: Math.min(400, width)

    readonly property string authorDisplayLine: {
        let _reloadTick = root.usersModel ? root.usersModel.loadStatus : 0
        let login = root.author || ""
        let nick = ""
        if (root.usersModel && typeof root.usersModel.displayNameForUsername === "function") {
            nick = root.usersModel.displayNameForUsername(login) || ""
        }
        if (root.hideUsernames) {
            if (nick.length > 0)
                return nick
            return login.length > 0 ? login : "unknown"
        }
        let base = login.length > 0 ? login : "unknown"
        if (nick.length > 0 && nick.toLowerCase() !== login.toLowerCase())
            return nick + " @" + base
        return base
    }

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

    implicitHeight: msgColumn.implicitHeight + 12
    Rectangle {
        id: messagePane
        readonly property bool mineToRight: !root.isWide && root.isMineMessage
        //anchors.left: mineToRight ? undefined : parent.left
        //anchors.right: mineToRight ? parent.right : undefined
        x: mineToRight ? parent.width - width : 0
        width: msgColumn.width + msgColumn.x
        height: msgColumn.height + msgColumn.anchors.topMargin + msgColumn.anchors.bottomMargin
        radius: 12
        bottomLeftRadius: mineToRight ? radius : 2
        bottomRightRadius: mineToRight ? 2 : radius

        Column {
            id: msgColumn
            spacing: 2

            Label {
                text: root.authorDisplayLine
                color: "#9aa4b2"
                font.pixelSize: 11
                elide: Label.ElideRight
                leftPadding: messagePane.topLeftRadius
                rightPadding: messagePane.topRightRadius
            }
            Rectangle {
                id: quoteBlock
                visible: root.quotePreviewText.length > 0
                width: root.messageMaxWidth
                height: visible ? quoteLabel.implicitHeight + 12 : 0
                color: "#2a3344"
                radius: 6

                Rectangle {
                    width: 3
                    height: parent.height - 8
                    anchors.left: parent.left
                    anchors.leftMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 1
                    color: "#4d78ff"
                }

                Label {
                    id: quoteLabel
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 14
                    anchors.rightMargin: 8
                    maximumLineCount: 1
                    text: root.quotePreviewText
                    color: "#9aa4b2"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.quoteClicked(root.quotedMessageId)
                }
            }
            Item {
                width: messageText.width
                height: messageText.contentHeight
                TextEdit {
                    id: measureText
                    textMargin: messageText.textMargin
                    text: messageText.text
                    bottomPadding: messageText.bottomPadding
                    readOnly: true
                    visible: false
                    width: root.messageMaxWidth
                    textFormat: messageText.textFormat
                    wrapMode: TextEdit.Wrap
                }

                TextEdit {
                    id: messageText
                    textMargin: 8
                    width: Math.min(root.messageMaxWidth, measureText.contentWidth)
                    text: root.text || "(empty message)"
                    wrapMode: TextEdit.Wrap
                    bottomPadding: 0
                    readOnly: true
                    selectByMouse: true
                    textFormat: TextEdit.MarkdownText
                    onLinkActivated: link => Qt.openUrlExternally(link)
                    ToolTip {
                        text: messageText.hoveredLink
                        visible: !!text
                        delay: 1000
                    }
                }
            }
        }
    }
}
